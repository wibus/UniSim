/**
 * Copyright (c) 2017 Eric Bruneton
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holders nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

/*<h2>atmosphere/model.cc</h2>

<p>This file implements the <a href="model.h.html">API of our atmosphere
model</a>. Its main role is to precompute the transmittance, scattering and
irradiance textures. The GLSL functions to precompute them are provided in
<a href="functions.glsl.html">functions.glsl</a>, but they are not sufficient.
They must be used in fully functional shaders and programs, and these programs
must be called in the correct order, with the correct input and output textures
(via framebuffer objects), to precompute each scattering order in sequence, as
described in Algorithm 4.1 of
<a href="https://hal.inria.fr/inria-00288758/en">our paper</a>. This is the role
of the following C++ code.
*/

#include "model.h"

#include <cassert>
#include <cmath>
#include <iostream>
#include <memory>

#include <resource/bruneton/constants.h>
#include <resource/bruneton/definitions.h>
#include <resource/sky.h>

#include <graphic/gpudevice.h>

#include <engine/scene.h>

/*
<p>The rest of this file is organized in 3 parts:
<ul>
<li>the <a href="#shaders">first part</a> defines the shaders used to precompute
the atmospheric textures,</li>
<li>the <a href="#utilities">second part</a> provides utility classes and
functions used to compile shaders, create textures, draw quads, etc,</li>
<li>the <a href="#implementation">third part</a> provides the actual
implementation of the <code>Model</code> class, using the above tools.</li>
</ul>

<h3 id="shaders">Shader definitions</h3>

<p>In order to precompute a texture we attach it to a framebuffer object (FBO)
and we render a full quad in this FBO. For this we need a basic vertex shader:
*/

namespace unisim
{
namespace bruneton
{

DefineResource(BrunetonTransmittance);
DefineResource(BrunetonScattering);
DefineResource(BrunetonSingleMieScattering);
DefineResource(BrunetonIrradiance);

DefineResource(BrunetonDeltaIrradiance);
DefineResource(BrunetonDeltaRayleighScattering);
DefineResource(BrunetonDeltaMieScattering);
DefineResource(BrunetonDeltaScatteringDensity);
DefineResource(BrunetonDeltaMultipleScattering); // Can be merged with BrunetonDeltaRayleighScattering

namespace
{

/*
<p>and a fragment shader, which depends on the texture we want to compute. This
is the role of the following shaders, which simply wrap the precomputation
functions from <a href="functions.glsl.html">functions.glsl</a> in complete
shaders (with a <code>main</code> function and a proper declaration of the
shader inputs and outputs). Note that these strings must be concatenated with
<code>definitions.glsl</code> and <code>functions.glsl</code> (provided as C++
string literals by the generated <code>.glsl.inc</code> files), as well as with
a definition of the <code>ATMOSPHERE</code> constant - containing the atmosphere
parameters, to really get a complete shader. Note also the
<code>luminance_from_radiance</code> uniforms: these are used in precomputed
illuminance mode to convert the radiance values computed by the
<code>functions.glsl</code> functions to luminance values (see the
<code>Init</code> method for more details).
*/

#include <shaders/bruneton/definitions.glsl.inc>
#include <shaders/bruneton/functions.glsl.inc>

const char kComputeTransmittanceShader[] =
    R"(
    layout(binding = 0, rgba32f) uniform image2D transmittance_image;

    layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;
    void main()
    {
        ivec2 threadID = ivec2(gl_GlobalInvocationID.xy);
        ivec2 dimensions = imageSize(transmittance_image);
        if (all(lessThan(threadID, dimensions)))
        {
            vec2 fragCoord = vec2(threadID) + vec2(0.5);

            vec3 transmittance = ComputeTransmittanceToTopAtmosphereBoundaryTexture(ATMOSPHERE, fragCoord);
            imageStore(transmittance_image, threadID, vec4(transmittance, 0));
        }
    }
)";

const char kComputeDirectIrradianceShader[] =
R"(
    layout(binding = 0, rgba32f) uniform image2D delta_irradiance_texture;
    layout(binding = 1, rgba32f) uniform image2D irradiance_texture;

    uniform int blend;
    uniform sampler2D transmittance_texture;

    layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;
    void main()
    {
        ivec2 threadID = ivec2(gl_GlobalInvocationID.xy);
        ivec2 dimensions = imageSize(delta_irradiance_texture);

        if (all(lessThan(threadID, dimensions)))
        {
            vec2 fragCoord = vec2(threadID) + vec2(0.5);

            IrradianceSpectrum delta_irradiance = ComputeDirectIrradianceTexture(ATMOSPHERE, transmittance_texture, fragCoord);
            imageStore(delta_irradiance_texture, threadID, vec4(delta_irradiance, 0.0));

            if (blend == 0)
                imageStore(irradiance_texture, threadID, vec4(0.0));
        }
    }
)";

const char kComputeSingleScatteringShader[] =
R"(
    layout(binding = 0, rgba32f) uniform image3D delta_rayleigh_scattering_texture;
    layout(binding = 1, rgba32f) uniform image3D delta_mie_scattering_texture;
    layout(binding = 2, rgba32f) uniform image3D scattering_texture;
    layout(binding = 3, rgba32f) uniform image3D single_mie_scattering_texture;

    uniform int blend;
    uniform mat3 luminance_from_radiance;
    uniform sampler2D transmittance_texture;

    layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;
    void main()
    {
        ivec3 threadID = ivec3(gl_GlobalInvocationID.xyz);
        ivec3 dimensions = imageSize(scattering_texture);

        if (all(lessThan(threadID, dimensions)))
        {
            vec3 fragCoord = vec3(threadID) + vec3(0.5);

            vec3 delta_rayleigh;
            vec3 delta_mie;
            ComputeSingleScatteringTexture(ATMOSPHERE, transmittance_texture, fragCoord, delta_rayleigh, delta_mie);

            imageStore(delta_rayleigh_scattering_texture, threadID, vec4(delta_rayleigh, 0.0));
            imageStore(delta_mie_scattering_texture, threadID, vec4(delta_mie, 0.0));

            vec3 scattering = luminance_from_radiance * delta_rayleigh.rgb;
            if (blend == 1)
            {
                scattering += imageLoad(scattering_texture, threadID).rgb;
            }
            imageStore(scattering_texture, threadID, vec4(scattering, 0.0));

            vec3 single_mie_scattering = luminance_from_radiance * delta_mie;
            if (blend == 1)
            {
                single_mie_scattering += imageLoad(single_mie_scattering_texture, threadID).rgb;
            }
            imageStore(single_mie_scattering_texture, threadID, vec4(single_mie_scattering, 0.0));
        }
    }
)";

const char kComputeScatteringDensityShader[] =
R"(
    layout(binding = 0, rgba32f) uniform image3D delta_scattering_density_texture;

    uniform sampler2D transmittance_texture;
    uniform sampler3D single_rayleigh_scattering_texture;
    uniform sampler3D single_mie_scattering_texture;
    uniform sampler3D multiple_scattering_texture;
    uniform sampler2D irradiance_texture;

    uniform int scattering_order;

    layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;
    void main()
    {
        ivec3 threadID = ivec3(gl_GlobalInvocationID.xyz);
        ivec3 dimensions = imageSize(delta_scattering_density_texture);

        if (all(lessThan(threadID, dimensions)))
        {
            vec3 fragCoord = vec3(threadID) + vec3(0.5);

            vec3 scattering_density = ComputeScatteringDensityTexture(
                ATMOSPHERE, transmittance_texture, single_rayleigh_scattering_texture,
                single_mie_scattering_texture, multiple_scattering_texture,
                irradiance_texture, fragCoord,
                scattering_order);

            imageStore(delta_scattering_density_texture, threadID, vec4(scattering_density, 0.0));
        }
    }
)";

const char kComputeIndirectIrradianceShader[] =
R"(
    layout(binding = 0, rgba32f) uniform image2D delta_irradiance_texture;
    layout(binding = 1, rgba32f) uniform image2D irradiance_texture;

    uniform sampler3D single_rayleigh_scattering_texture;
    uniform sampler3D single_mie_scattering_texture;
    uniform sampler3D multiple_scattering_texture;
    uniform mat3 luminance_from_radiance;
    uniform int scattering_order;

    layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;
    void main()
    {
        ivec2 threadID = ivec2(gl_GlobalInvocationID.xy);
        ivec2 dimensions = imageSize(irradiance_texture);

        if (all(lessThan(threadID, dimensions)))
        {
            vec2 fragCoord = vec2(threadID) + vec2(0.5);

            vec3 delta_irradiance = ComputeIndirectIrradianceTexture(
                ATMOSPHERE, single_rayleigh_scattering_texture,
                single_mie_scattering_texture, multiple_scattering_texture,
                fragCoord, scattering_order);
            imageStore(delta_irradiance_texture, threadID, vec4(delta_irradiance, 0.0));

            vec3 irradiance = luminance_from_radiance * delta_irradiance;
            irradiance += imageLoad(irradiance_texture, threadID).rgb;
            imageStore(irradiance_texture, threadID, vec4(irradiance, 0.0));
        }
    }
)";

const char kComputeMultipleScatteringShader[] =
R"(
    layout(binding = 0, rgba32f) uniform image3D delta_multiple_scattering_texture;
    layout(binding = 1, rgba32f) uniform image3D scattering_texture;

    uniform sampler2D transmittance_texture;
    uniform sampler3D scattering_density_texture;
    uniform mat3 luminance_from_radiance;

    layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;
    void main()
    {
        ivec3 threadID = ivec3(gl_GlobalInvocationID.xyz);
        ivec3 dimensions = imageSize(scattering_texture);

        if (all(lessThan(threadID, dimensions)))
        {
            vec3 fragCoord = vec3(threadID) + vec3(0.5);

            float nu;
            vec3 delta_multiple_scattering = ComputeMultipleScatteringTexture(
                ATMOSPHERE, transmittance_texture, scattering_density_texture, fragCoord, nu);
            imageStore(delta_multiple_scattering_texture, threadID, vec4(delta_multiple_scattering, 0.0));

            vec4 scattering = vec4(luminance_from_radiance * delta_multiple_scattering.rgb / RayleighPhaseFunction(nu), 0.0);
            scattering += imageLoad(scattering_texture, threadID);
            imageStore(scattering_texture, threadID, scattering);
        }
    }
)";

/*
<p>We finally need a shader implementing the GLSL functions exposed in our API,
which can be done by calling the corresponding functions in
<a href="functions.glsl.html#rendering">functions.glsl</a>, with the precomputed
texture arguments taken from uniform variables (note also the
*<code>_RADIANCE_TO_LUMINANCE</code> conversion constants in the last functions:
they are computed in the <a href="#utilities">second part</a> below, and their
definitions are concatenated to this GLSL code to get a fully functional
shader).
*/

const char kAtmosphereShader[] =
R"(
    uniform sampler2D transmittance_texture;
    uniform sampler3D scattering_texture;
    uniform sampler3D single_mie_scattering_texture;
    uniform sampler2D irradiance_texture;
    #ifdef RADIANCE_API_ENABLED
    RadianceSpectrum GetSolarRadiance() {
      return ATMOSPHERE.solar_irradiance /
          (PI * ATMOSPHERE.sun_angular_radius * ATMOSPHERE.sun_angular_radius);
    }
    RadianceSpectrum GetSkyRadiance(
        Position camera, Direction view_ray, Length shadow_length,
        Direction sun_direction, out DimensionlessSpectrum transmittance) {
      return GetSkyRadiance(ATMOSPHERE, transmittance_texture,
          scattering_texture, single_mie_scattering_texture,
          camera, view_ray, shadow_length, sun_direction, transmittance);
    }
    RadianceSpectrum GetSkyRadianceToPoint(
        Position camera, Position point, Length shadow_length,
        Direction sun_direction, out DimensionlessSpectrum transmittance) {
      return GetSkyRadianceToPoint(ATMOSPHERE, transmittance_texture,
          scattering_texture, single_mie_scattering_texture,
          camera, point, shadow_length, sun_direction, transmittance);
    }
    IrradianceSpectrum GetSunAndSkyIrradiance(
       Position p, Direction normal, Direction sun_direction,
       out IrradianceSpectrum sky_irradiance) {
      return GetSunAndSkyIrradiance(ATMOSPHERE, transmittance_texture,
          irradiance_texture, p, normal, sun_direction, sky_irradiance);
    }
    #endif
    Luminance3 GetSolarLuminance() {
      return ATMOSPHERE.solar_irradiance /
          (PI * ATMOSPHERE.sun_angular_radius * ATMOSPHERE.sun_angular_radius) *
          SUN_SPECTRAL_RADIANCE_TO_LUMINANCE;
    }
    Luminance3 GetSkyLuminance(
        Position camera, Direction view_ray, Length shadow_length,
        Direction sun_direction, out DimensionlessSpectrum transmittance) {
      return GetSkyRadiance(ATMOSPHERE, transmittance_texture,
          scattering_texture, single_mie_scattering_texture,
          camera, view_ray, shadow_length, sun_direction, transmittance) *
          SKY_SPECTRAL_RADIANCE_TO_LUMINANCE;
    }
    Luminance3 GetSkyLuminanceToPoint(
        Position camera, Position point, Length shadow_length,
        Direction sun_direction, out DimensionlessSpectrum transmittance) {
      return GetSkyRadianceToPoint(ATMOSPHERE, transmittance_texture,
          scattering_texture, single_mie_scattering_texture,
          camera, point, shadow_length, sun_direction, transmittance) *
          SKY_SPECTRAL_RADIANCE_TO_LUMINANCE;
    }
    Illuminance3 GetSunAndSkyIlluminance(
       Position p, Direction normal, Direction sun_direction,
       out IrradianceSpectrum sky_irradiance) {
      IrradianceSpectrum sun_irradiance = GetSunAndSkyIrradiance(
          ATMOSPHERE, transmittance_texture, irradiance_texture, p, normal,
          sun_direction, sky_irradiance);
      sky_irradiance *= SKY_SPECTRAL_RADIANCE_TO_LUMINANCE;
      return sun_irradiance * SUN_SPECTRAL_RADIANCE_TO_LUMINANCE;
    }
)";

/*<h3 id="utilities">Utility classes and functions</h3>

<p>To compile and link these shaders into programs, and to set their uniforms,
we use the following utility class:
*/

class Program
{
public:
    Program(const std::string &compute_shader_source)
    {
        program_ = glCreateProgram();

        const char *source;
        source = compute_shader_source.c_str();
        GLuint compute_shader = glCreateShader(GL_COMPUTE_SHADER);
        glShaderSource(compute_shader, 1, &source, NULL);
        glCompileShader(compute_shader);
        CheckShader(compute_shader);
        glAttachShader(program_, compute_shader);

        glLinkProgram(program_);
        CheckProgram(program_);

        glDetachShader(program_, compute_shader);
        glDeleteShader(compute_shader);
    }

    ~Program() { glDeleteProgram(program_); }

    void Use() const { glUseProgram(program_); }

    void BindMat3(const std::string &uniform_name, const std::array<float, 9> &value) const
    {
        glUniformMatrix3fv(glGetUniformLocation(program_, uniform_name.c_str()),
                           1,
                           true /* transpose */,
                           value.data());
    }

    void BindInt(const std::string &uniform_name, int value) const
    {
        glUniform1i(glGetUniformLocation(program_, uniform_name.c_str()), value);
    }

    void BindTexture2d(const std::string &sampler_uniform_name,
                       GLuint texture,
                       GLuint texture_unit) const
    {
        glActiveTexture(GL_TEXTURE0 + texture_unit);
        glBindTexture(GL_TEXTURE_2D, texture);
        BindInt(sampler_uniform_name, texture_unit);
    }

    void BindTexture3d(const std::string &sampler_uniform_name,
                       GLuint texture,
                       GLuint texture_unit) const
    {
        glActiveTexture(GL_TEXTURE0 + texture_unit);
        glBindTexture(GL_TEXTURE_3D, texture);
        BindInt(sampler_uniform_name, texture_unit);
    }

    void BindImage(const std::string &sampler_uniform_name,
                   GLuint texture,
                   GLuint image_unit,
                   GLenum target) const
    {
        const bool layered = target == GL_TEXTURE_3D;
        const GLint layer = 0;
        glBindImageTexture(image_unit,
                           texture,
                           0,
                           layered,
                           layer,
                           GL_READ_WRITE,
                           Model::internalFormat());

        if (glGetError() != GL_NO_ERROR) {
            std::cout << "Bad image binding" << std::endl;
        }
    }

private:
    static void CheckShader(GLuint shader)
    {
        GLint compile_status;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compile_status);
        if (compile_status == GL_FALSE) {
            PrintShaderLog(shader);
        }
        assert(compile_status == GL_TRUE);
    }

    static void PrintShaderLog(GLuint shader)
    {
        GLint log_length;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_length);
        if (log_length > 0) {
            std::unique_ptr<char[]> log_data(new char[log_length]);
            glGetShaderInfoLog(shader, log_length, &log_length, log_data.get());
            std::cerr << "compile log = " << std::string(log_data.get(), log_length) << std::endl;
        }
    }

    static void CheckProgram(GLuint program)
    {
        GLint link_status;
        glGetProgramiv(program, GL_LINK_STATUS, &link_status);
        if (link_status == GL_FALSE) {
            PrintProgramLog(program);
        }
        assert(link_status == GL_TRUE);
        assert(glGetError() == 0);
    }

    static void PrintProgramLog(GLuint program)
    {
        GLint log_length;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_length);
        if (log_length > 0) {
            std::unique_ptr<char[]> log_data(new char[log_length]);
            glGetProgramInfoLog(program, log_length, &log_length, log_data.get());
            std::cerr << "link log = " << std::string(log_data.get(), log_length) << std::endl;
        }
    }

    GLuint program_;
};

void dispatch(uint threadCountX,  uint threadCountY,  uint threadCountZ=1,
              uint threadSizeX=8, uint threadSizeY=8, uint threadSizeZ=1)
{
    glDispatchCompute((threadCountX + threadSizeX-1) / threadSizeX,
                      (threadCountY + threadSizeY-1) / threadSizeY,
                      (threadCountZ + threadSizeZ-1) / threadSizeZ);
}

} // anonymous namespace

/*<h3 id="implementation">Model implementation</h3>

<p>Using the above utility functions and classes, we can now implement the
constructor of the <code>Model</code> class. This constructor generates a piece
of GLSL code that defines an <code>ATMOSPHERE</code> constant containing the
atmosphere parameters (we use constants instead of uniforms to enable constant
folding and propagation optimizations in the GLSL compiler), concatenated with
<a href="functions.glsl.html">functions.glsl</a>, and with
<code>kAtmosphereShader</code>, to get the shader exposed by our API in
<code>GetShader</code>. It also allocates the precomputed textures (but does not
initialize them), as well as a vertex buffer object to render a full screen quad
(used to render into the precomputed textures).
*/

Model::Model(GraphicContext& context, bool precomputed_luminance)
    // The number of wavelengths for which atmospheric scattering must be
    // precomputed (the temporary GPU memory used during precomputations, and
    // the GPU memory used by the precomputed results, is independent of this
    // number, but the <i>precomputation time is directly proportional to this
    // number</i>):
    // - if this number is less than or equal to 3, scattering is precomputed
    // for 3 wavelengths, and stored as irradiance values. Then both the
    // radiance-based and the luminance-based API functions are provided (see
    // the above note).
    // - otherwise, scattering is precomputed for this number of wavelengths
    // (rounded up to a multiple of 3), integrated with the CIE color matching
    // functions, and stored as illuminance values. Then only the
    // luminance-based API functions are provided (see the above note).
    : PathTracerProviderTask("Bruneton Sky Model")
    , num_precomputed_wavelengths_(precomputed_luminance ? 15 : 3)
    , _isDirty(true)
{
    const Atmosphere::Params& params = context.scene.sky()->atmosphere().get()->params();

    auto profile = [](ParameterDensityProfileLayer layer) {
        return DensityProfileLayer(layer.width.to(m),
                                   layer.exp_term(), layer.exp_scale.to(1.0 / m),
                                   layer.linear_term.to(1.0 / m), layer.constant_term());
    };

    constexpr Length kLengthUnit = 1.0 * km;

    Init(
        context,
        params.wavelengths,
        params.solar_irradiance.to(
            watt_per_square_meter_per_nm),
        params.sun_angular_radius.to(rad),
        params.bottom_radius.to(m),
        params.top_radius.to(m),
        {profile(params.rayleigh_density.layers[1])},
        params.rayleigh_scattering.to(1.0 / m),
        {profile(params.mie_density.layers[1])},
        params.mie_scattering.to(1.0 / m),
        params.mie_extinction.to(1.0 / m),
        params.mie_phase_function_g(),
        {profile(params.absorption_density.layers[0]),
         profile(params.absorption_density.layers[1])},
        params.absorption_extinction.to(1.0 / m),
        params.ground_albedo.to(Number::Unit()),
        acos(params.mu_s_min()),
        kLengthUnit.to(m));

    _isDirty = true;
}


/*
<p>The destructor is trivial:
*/

Model::~Model()
{
    glDeleteShader(atmosphere_shader_);
}

bool Model::defineResources(GraphicContext& context)
{
    GpuResourceManager& resources = context.resources;

    bool ok = true;

    ok = ok && resources.define<GpuImageResource>(ResourceName(BrunetonTransmittance), {
        .width  = TRANSMITTANCE_TEXTURE_WIDTH,
        .height = TRANSMITTANCE_TEXTURE_HEIGHT,
        .depth  = 1,
        .format = Model::internalFormat()
    });
    ok = ok && resources.define<GpuImageResource>(ResourceName(BrunetonScattering), {
        .width  = SCATTERING_TEXTURE_WIDTH,
        .height = SCATTERING_TEXTURE_HEIGHT,
        .depth  = SCATTERING_TEXTURE_DEPTH,
        .format = Model::internalFormat()
    });
    ok = ok && resources.define<GpuImageResource>(ResourceName(BrunetonSingleMieScattering), {
        .width  = SCATTERING_TEXTURE_WIDTH,
        .height = SCATTERING_TEXTURE_HEIGHT,
        .depth  = SCATTERING_TEXTURE_DEPTH,
        .format = Model::internalFormat()
    });
    ok = ok && resources.define<GpuImageResource>(ResourceName(BrunetonIrradiance), {
        .width  = IRRADIANCE_TEXTURE_WIDTH,
        .height = IRRADIANCE_TEXTURE_HEIGHT,
        .depth  = 1,
        .format = Model::internalFormat()
    });

    ok = ok && resources.define<GpuImageResource>(ResourceName(BrunetonDeltaIrradiance), {
        .width  = IRRADIANCE_TEXTURE_WIDTH,
        .height = IRRADIANCE_TEXTURE_HEIGHT,
        .depth  = 1,
        .format = Model::internalFormat()
    });
    ok = ok && resources.define<GpuImageResource>(ResourceName(BrunetonDeltaRayleighScattering), {
        .width  = SCATTERING_TEXTURE_WIDTH,
        .height = SCATTERING_TEXTURE_HEIGHT,
        .depth  = SCATTERING_TEXTURE_DEPTH,
        .format = Model::internalFormat()
    });
    ok = ok && resources.define<GpuImageResource>(ResourceName(BrunetonDeltaMieScattering), {
        .width  = SCATTERING_TEXTURE_WIDTH,
        .height = SCATTERING_TEXTURE_HEIGHT,
        .depth  = SCATTERING_TEXTURE_DEPTH,
        .format = Model::internalFormat()
    });
    ok = ok && resources.define<GpuImageResource>(ResourceName(BrunetonDeltaScatteringDensity), {
        .width  = SCATTERING_TEXTURE_WIDTH,
        .height = SCATTERING_TEXTURE_HEIGHT,
        .depth  = SCATTERING_TEXTURE_DEPTH,
        .format = Model::internalFormat()
    });
    ok = ok && resources.define<GpuImageResource>(ResourceName(BrunetonDeltaMultipleScattering), {
        .width  = SCATTERING_TEXTURE_WIDTH,
        .height = SCATTERING_TEXTURE_HEIGHT,
        .depth  = SCATTERING_TEXTURE_DEPTH,
        .format = Model::internalFormat()
    });

    return ok;
}

bool Model::defineShaders(GraphicContext& context)
{
    return true;
}

bool Model::definePathTracerModules(
    GraphicContext& context,
    std::vector<std::shared_ptr<PathTracerModule>>& modules)
{
    std::shared_ptr<GraphicShader> modelShader(new GraphicShader("Sky Model", std::move(GraphicShaderHandle(atmosphere_shader_, false))));
    std::shared_ptr<PathTracerModule> modelModule(new PathTracerModule("Sky Model", modelShader));
    modules.push_back(modelModule);

    return true;
}

bool Model::definePathTracerInterface(
    GraphicContext& context,
    PathTracerInterface& interface)
{
    bool ok = true;

    // Bruneton
    ok = ok && interface.declareTexture({"transmittance_texture"});
    ok = ok && interface.declareTexture({"scattering_texture"});
    //ok = ok && interface.declareTexture({"irradiance_texture"});
    ok = ok && interface.declareTexture({"single_mie_scattering_texture"});

    return ok;
}

void Model::bindPathTracerResources(
    GraphicContext& context,
    CompiledGpuProgramInterface& compiledGpi) const
{
    GpuResourceManager& resources = context.resources;

    context.device.bindTexture(
        resources.get<GpuImageResource>(ResourceName(BrunetonTransmittance)),
        compiledGpi.getTextureBindPoint("transmittance_texture"));

    context.device.bindTexture(
        resources.get<GpuImageResource>(ResourceName(BrunetonScattering)),
        compiledGpi.getTextureBindPoint("scattering_texture"));

    //context.device.bindTexture(
    //    resources.get<GpuImageResource>(ResourceName(BrunetonIrradiance)),
    //    compiledGpi.getTextureBindPoint("irradiance_texture"));

    context.device.bindTexture(
        resources.get<GpuImageResource>(ResourceName(BrunetonSingleMieScattering)),
        compiledGpi.getTextureBindPoint("single_mie_scattering_texture"));
}

void Model::update(GraphicContext& context)
{
}

void Model::render(GraphicContext& context)
{
    if (_isDirty)
    {
        Recompute(context);
        _isDirty = false;
    }
}

void Model::Init(
    GraphicContext& context,
    const std::vector<double> &wavelengths,
    const std::vector<double> &solar_irradiance,
    const double sun_angular_radius,
    double bottom_radius,
    double top_radius,
    const std::vector<DensityProfileLayer> &rayleigh_density,
    const std::vector<double> &rayleigh_scattering,
    const std::vector<DensityProfileLayer> &mie_density,
    const std::vector<double> &mie_scattering,
    const std::vector<double> &mie_extinction,
    double mie_phase_function_g,
    const std::vector<DensityProfileLayer> &absorption_density,
    const std::vector<double> &absorption_extinction,
    const std::vector<double> &ground_albedo,
    double max_sun_zenith_angle,
    double length_unit_in_meters)
{
    auto to_string = [&wavelengths](const std::vector<double> &v, const vec3 &lambdas, double scale)
    {
        double r = Interpolate(wavelengths, v, lambdas[0]) * scale;
        double g = Interpolate(wavelengths, v, lambdas[1]) * scale;
        double b = Interpolate(wavelengths, v, lambdas[2]) * scale;
        return "vec3(" + std::to_string(r) + "," + std::to_string(g) + "," + std::to_string(b)
            + ")";
    };

    auto density_layer = [length_unit_in_meters](const DensityProfileLayer& layer)
    {
        return "DensityProfileLayer(" + std::to_string(layer.width / length_unit_in_meters) + ","
               + std::to_string(layer.exp_term) + ","
               + std::to_string(layer.exp_scale * length_unit_in_meters) + ","
               + std::to_string(layer.linear_term * length_unit_in_meters) + ","
               + std::to_string(layer.constant_term) + ")";
    };

    auto density_profile = [density_layer](std::vector<DensityProfileLayer> layers)
    {
        constexpr int kLayerCount = 2;
        while (layers.size() < kLayerCount)
        {
            layers.insert(layers.begin(), DensityProfileLayer());
        }

        std::string result = "DensityProfile(DensityProfileLayer[" + std::to_string(kLayerCount) + "](";
        for (int i = 0; i < kLayerCount; ++i)
        {
            result += density_layer(layers[i]);
            result += i < kLayerCount - 1 ? "," : "))";
        }

        return result;
    };

    // Compute the values for the SKY_RADIANCE_TO_LUMINANCE constant. In theory
    // this should be 1 in precomputed illuminance mode (because the precomputed
    // textures already contain illuminance values). In practice, however, storing
    // true illuminance values in half precision textures yields artefacts
    // (because the values are too large), so we store illuminance values divided
    // by MAX_LUMINOUS_EFFICACY instead. This is why, in precomputed illuminance
    // mode, we set SKY_RADIANCE_TO_LUMINANCE to MAX_LUMINOUS_EFFICACY.
    bool precompute_illuminance = num_precomputed_wavelengths_ > 3;
    double sky_k_r, sky_k_g, sky_k_b;
    if (precompute_illuminance)
    {
        sky_k_r = sky_k_g = sky_k_b = MAX_LUMINOUS_EFFICACY;
    }
    else
    {
        ComputeSpectralRadianceToLuminanceFactors(wavelengths,
                solar_irradiance,
                -3 /* lambda_power */,
                &sky_k_r,
                &sky_k_g,
                &sky_k_b);
    }
    // Compute the values for the SUN_RADIANCE_TO_LUMINANCE constant.
    double sun_k_r, sun_k_g, sun_k_b;
    ComputeSpectralRadianceToLuminanceFactors(wavelengths,
            solar_irradiance,
            0 /* lambda_power */,
            &sun_k_r,
            &sun_k_g,
            &sun_k_b);

    // A lambda that creates a GLSL header containing our atmosphere computation
    // functions, specialized for the given atmosphere parameters and for the 3
    // wavelengths in 'lambdas'.
    glsl_header_factory_ = [=](const vec3& lambdas)
    {
        return "#version 440\n"
               "#define IN(x) const in x\n"
               "#define OUT(x) out x\n"
               "#define TEMPLATE(x)\n"
               "#define TEMPLATE_ARGUMENT(x)\n"
               "#define assert(x)\n"
               "const int TRANSMITTANCE_TEXTURE_WIDTH = "
               + std::to_string(TRANSMITTANCE_TEXTURE_WIDTH) + ";\n"
               + "const int TRANSMITTANCE_TEXTURE_HEIGHT = "
               + std::to_string(TRANSMITTANCE_TEXTURE_HEIGHT) + ";\n"
               + "const int SCATTERING_TEXTURE_R_SIZE = "
               + std::to_string(SCATTERING_TEXTURE_R_SIZE) + ";\n"
               + "const int SCATTERING_TEXTURE_MU_SIZE = "
               + std::to_string(SCATTERING_TEXTURE_MU_SIZE) + ";\n"
               + "const int SCATTERING_TEXTURE_MU_S_SIZE = "
               + std::to_string(SCATTERING_TEXTURE_MU_S_SIZE) + ";\n"
               + "const int SCATTERING_TEXTURE_NU_SIZE = "
               + std::to_string(SCATTERING_TEXTURE_NU_SIZE) + ";\n"
               + "const int IRRADIANCE_TEXTURE_WIDTH = " + std::to_string(IRRADIANCE_TEXTURE_WIDTH)
               + ";\n" + "const int IRRADIANCE_TEXTURE_HEIGHT = "
               + std::to_string(IRRADIANCE_TEXTURE_HEIGHT) + ";\n" + definitions_glsl
               + "const AtmosphereParameters ATMOSPHERE = AtmosphereParameters(\n"
               + to_string(solar_irradiance, lambdas, 1.0) + ",\n"
               + std::to_string(sun_angular_radius) + ",\n"
               + std::to_string(bottom_radius / length_unit_in_meters) + ",\n"
               + std::to_string(top_radius / length_unit_in_meters) + ",\n"
               + density_profile(rayleigh_density) + ",\n"
               + to_string(rayleigh_scattering, lambdas, length_unit_in_meters) + ",\n"
               + density_profile(mie_density) + ",\n"
               + to_string(mie_scattering, lambdas, length_unit_in_meters) + ",\n"
               + to_string(mie_extinction, lambdas, length_unit_in_meters) + ",\n"
               + std::to_string(mie_phase_function_g) + ",\n" + density_profile(absorption_density)
               + ",\n" + to_string(absorption_extinction, lambdas, length_unit_in_meters) + ",\n"
               + to_string(ground_albedo, lambdas, 1.0) + ",\n"
               + std::to_string(cos(max_sun_zenith_angle)) + ");\n"
               + "const vec3 SKY_SPECTRAL_RADIANCE_TO_LUMINANCE = vec3(" + std::to_string(sky_k_r)
               + "," + std::to_string(sky_k_g) + "," + std::to_string(sky_k_b) + ");\n"
               + "const vec3 SUN_SPECTRAL_RADIANCE_TO_LUMINANCE = vec3(" + std::to_string(sun_k_r)
               + "," + std::to_string(sun_k_g) + "," + std::to_string(sun_k_b) + ");\n"
               + functions_glsl;
    };

    // Create and compile the shader providing our API.
    std::string shader = glsl_header_factory_({kLambdaR, kLambdaG, kLambdaB})
                         + (precompute_illuminance ? "" : "#define RADIANCE_API_ENABLED\n")
                         + kAtmosphereShader;
    const char* source = shader.c_str();
    atmosphere_shader_ = glCreateShader(GL_COMPUTE_SHADER);
    glShaderSource(atmosphere_shader_, 1, &source, NULL);
    glCompileShader(atmosphere_shader_);
}

/*
<p>The Init method precomputes the atmosphere textures. It first allocates the
temporary resources it needs, then calls <code>Precompute</code> to do the
actual precomputations, and finally destroys the temporary resources.

<p>Note that there are two precomputation modes here, depending on whether we
want to store precomputed irradiance or illuminance values:
<ul>
  <li>In precomputed irradiance mode, we simply need to call
  <code>Precompute</code> with the 3 wavelengths for which we want to precompute
  irradiance, namely <code>kLambdaR</code>, <code>kLambdaG</code>,
  <code>kLambdaB</code> (with the identity matrix for
  <code>luminance_from_radiance</code>, since we don't want any conversion from
  radiance to luminance)</li>
  <li>In precomputed illuminance mode, we need to precompute irradiance for
  <code>num_precomputed_wavelengths_</code>, and then integrate the results,
  multiplied with the 3 CIE xyz color matching functions and the XYZ to sRGB
  matrix to get sRGB illuminance values.
  <p>A naive solution would be to allocate temporary textures for the
  intermediate irradiance results, then perform the integration from irradiance
  to illuminance and store the result in the final precomputed texture. In
  pseudo-code (and assuming one wavelength per texture instead of 3):
  <pre>
    create n temporary irradiance textures
    for each wavelength lambda in the n wavelengths:
       precompute irradiance at lambda into one of the temporary textures
    initializes the final illuminance texture with zeros
    for each wavelength lambda in the n wavelengths:
      accumulate in the final illuminance texture the product of the
      precomputed irradiance at lambda (read from the temporary textures)
      with the value of the 3 sRGB color matching functions at lambda (i.e.
      the product of the XYZ to sRGB matrix with the CIE xyz color matching
      functions).
  </pre>
  <p>However, this be would waste GPU memory. Instead, we can avoid allocating
  temporary irradiance textures, by merging the two above loops:
  <pre>
    for each wavelength lambda in the n wavelengths:
      accumulate in the final illuminance texture (or, for the first
      iteration, set this texture to) the product of the precomputed
      irradiance at lambda (computed on the fly) with the value of the 3
      sRGB color matching functions at lambda.
  </pre>
  <p>This is the method we use below, with 3 wavelengths per iteration instead
  of 1, using <code>Precompute</code> to compute 3 irradiances values per
  iteration, and <code>luminance_from_radiance</code> to multiply 3 irradiances
  with the values of the 3 sRGB color matching functions at 3 different
  wavelengths (yielding a 3x3 matrix).</li>
</ul>

<p>This yields the following implementation:
*/

void Model::Recompute(
    GraphicContext& context,
    unsigned int num_scattering_orders)
{
    GpuResourceManager& resources = context.resources;

    GLuint transmittance_texture = resources.get<GpuImageResource>(ResourceName(BrunetonTransmittance)).handle().texId;
    GLuint scattering_texture = resources.get<GpuImageResource>(ResourceName(BrunetonScattering)).handle().texId;
    GLuint irradiance_texture = resources.get<GpuImageResource>(ResourceName(BrunetonIrradiance)).handle().texId;
    GLuint single_mie_scattering_texture = resources.get<GpuImageResource>(ResourceName(BrunetonSingleMieScattering)).handle().texId;

    // The precomputations require temporary textures, in particular to store the
    // contribution of one scattering order, which is needed to compute the next
    // order of scattering (the final precomputed textures store the sum of all
    // the scattering orders). We allocate them here, and destroy them at the end
    // of this method.
    GLuint delta_irradiance_texture = resources.get<GpuImageResource>(ResourceName(BrunetonDeltaIrradiance)).handle().texId;
    GLuint delta_rayleigh_scattering_texture = resources.get<GpuImageResource>(ResourceName(BrunetonDeltaRayleighScattering)).handle().texId;
    GLuint delta_mie_scattering_texture = resources.get<GpuImageResource>(ResourceName(BrunetonDeltaMieScattering)).handle().texId;
    GLuint delta_scattering_density_texture = resources.get<GpuImageResource>(ResourceName(BrunetonDeltaScatteringDensity)).handle().texId;

    // delta_multiple_scattering_texture is only needed to compute scattering
    // order 3 or more, while delta_rayleigh_scattering_texture and
    // delta_mie_scattering_texture are only needed to compute double scattering.
    // Therefore, to save memory, we can store delta_rayleigh_scattering_texture
    // and delta_multiple_scattering_texture in the same GPU texture.
    GLuint delta_multiple_scattering_texture = delta_rayleigh_scattering_texture;

    // The actual precomputations depend on whether we want to store precomputed
    // irradiance or illuminance values.
    if (num_precomputed_wavelengths_ <= 3)
    {
        vec3 lambdas{kLambdaR, kLambdaG, kLambdaB};
        mat3 luminance_from_radiance{1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0};
        Precompute(
            transmittance_texture,
            scattering_texture,
            irradiance_texture,
            single_mie_scattering_texture,
            delta_irradiance_texture,
            delta_rayleigh_scattering_texture,
            delta_mie_scattering_texture,
            delta_scattering_density_texture,
            delta_multiple_scattering_texture,
            lambdas,
            luminance_from_radiance,
            false /* blend */,
            num_scattering_orders);
    }
    else
    {
        constexpr double kLambdaMin = 360.0;
        constexpr double kLambdaMax = 830.0;
        int num_iterations = (num_precomputed_wavelengths_ + 2) / 3;
        double dlambda = (kLambdaMax - kLambdaMin) / (3 * num_iterations);

        for (int i = 0; i < num_iterations; ++i)
        {
            vec3 lambdas{kLambdaMin + (3 * i + 0.5)* dlambda,
                         kLambdaMin + (3 * i + 1.5)* dlambda,
                         kLambdaMin + (3 * i + 2.5)* dlambda};
            auto coeff = [dlambda](double lambda, int component)
            {
                // Note that we don't include MAX_LUMINOUS_EFFICACY here, to avoid
                // artefacts due to too large values when using half precision on GPU.
                // We add this term back in kAtmosphereShader, via
                // SKY_SPECTRAL_RADIANCE_TO_LUMINANCE (see also the comments in the
                // Model constructor).
                double x = CieColorMatchingFunctionTableValue(lambda, 1);
                double y = CieColorMatchingFunctionTableValue(lambda, 2);
                double z = CieColorMatchingFunctionTableValue(lambda, 3);
                return static_cast<float>((XYZ_TO_SRGB[component * 3] * x
                                           + XYZ_TO_SRGB[component * 3 + 1] * y
                                           + XYZ_TO_SRGB[component * 3 + 2] * z)
                                          * dlambda);
            };
            mat3 luminance_from_radiance{coeff(lambdas[0], 0),
                                         coeff(lambdas[1], 0),
                                         coeff(lambdas[2], 0),
                                         coeff(lambdas[0], 1),
                                         coeff(lambdas[1], 1),
                                         coeff(lambdas[2], 1),
                                         coeff(lambdas[0], 2),
                                         coeff(lambdas[1], 2),
                                         coeff(lambdas[2], 2)};
            Precompute(
                transmittance_texture,
                scattering_texture,
                irradiance_texture,
                single_mie_scattering_texture,
                delta_irradiance_texture,
                delta_rayleigh_scattering_texture,
                delta_mie_scattering_texture,
                delta_scattering_density_texture,
                delta_multiple_scattering_texture,
                lambdas,
                luminance_from_radiance,
                i > 0 /* blend */,
                num_scattering_orders);
        }

        // After the above iterations, the transmittance texture contains the
        // transmittance for the 3 wavelengths used at the last iteration. But we
        // want the transmittance at kLambdaR, kLambdaG, kLambdaB instead, so we
        // must recompute it here for these 3 wavelengths:
        std::string header = glsl_header_factory_({kLambdaR, kLambdaG, kLambdaB});
        Program compute_transmittance(header + kComputeTransmittanceShader);
        compute_transmittance.Use();
        compute_transmittance.BindImage("transmittance_texture",
                                        transmittance_texture,
                                        0,
                                        GL_TEXTURE_2D);
        dispatch(TRANSMITTANCE_TEXTURE_WIDTH, TRANSMITTANCE_TEXTURE_HEIGHT);
        glMemoryBarrier(GL_ALL_BARRIER_BITS);
    }

    // Delete the temporary resources allocated at the begining of this method.
    glUseProgram(0);
    glDeleteTextures(1, &delta_scattering_density_texture);
    glDeleteTextures(1, &delta_mie_scattering_texture);
    glDeleteTextures(1, &delta_rayleigh_scattering_texture);
    glDeleteTextures(1, &delta_irradiance_texture);
    assert(glGetError() == 0);
}

/*
<p>Finally, we provide the actual implementation of the precomputation algorithm
described in Algorithm 4.1 of
<a href="https://hal.inria.fr/inria-00288758/en">our paper</a>. Each step is
explained by the inline comments below.
*/
void Model::Precompute(
    GLuint transmittance_texture,
    GLuint scattering_texture,
    GLuint irradiance_texture,
    GLuint single_mie_scattering_texture,
    GLuint delta_irradiance_texture,
    GLuint delta_rayleigh_scattering_texture,
    GLuint delta_mie_scattering_texture,
    GLuint delta_scattering_density_texture,
    GLuint delta_multiple_scattering_texture,
    const vec3& lambdas,
    const mat3& luminance_from_radiance,
    bool blend,
    unsigned int num_scattering_orders)
{
    // The precomputations require specific GLSL programs, for each precomputation
    // step. We create and compile them here (they are automatically destroyed
    // when this method returns, via the Program destructor).
    std::string header = glsl_header_factory_(lambdas);
    Program compute_transmittance(header + kComputeTransmittanceShader);
    Program compute_direct_irradiance(header + kComputeDirectIrradianceShader);
    Program compute_single_scattering(header + kComputeSingleScatteringShader);
    Program compute_scattering_density(header + kComputeScatteringDensityShader);
    Program compute_indirect_irradiance(header + kComputeIndirectIrradianceShader);
    Program compute_multiple_scattering(header + kComputeMultipleScatteringShader);

    // Compute the transmittance, and store it in transmittance_texture_.
    compute_transmittance.Use();
    compute_transmittance.BindImage("transmittance_texture", transmittance_texture, 0, GL_TEXTURE_2D);
    dispatch(TRANSMITTANCE_TEXTURE_WIDTH, TRANSMITTANCE_TEXTURE_HEIGHT);
    glMemoryBarrier(GL_ALL_BARRIER_BITS);

    // Compute the direct irradiance, store it in delta_irradiance_texture and,
    // depending on 'blend', either initialize irradiance_texture_ with zeros or
    // leave it unchanged (we don't want the direct irradiance in
    // irradiance_texture_, but only the irradiance from the sky).
    compute_direct_irradiance.Use();
    compute_direct_irradiance.BindInt("blend", blend ? 1 : 0);
    compute_direct_irradiance.BindTexture2d("transmittance_texture", transmittance_texture, 0);
    compute_direct_irradiance.BindImage("delta_irradiance_texture", delta_irradiance_texture, 0, GL_TEXTURE_2D);
    compute_direct_irradiance.BindImage("irradiance_texture", irradiance_texture, 1, GL_TEXTURE_2D);
    dispatch(IRRADIANCE_TEXTURE_WIDTH, IRRADIANCE_TEXTURE_WIDTH);
    glMemoryBarrier(GL_ALL_BARRIER_BITS);

    // Compute the rayleigh and mie single scattering, store them in
    // delta_rayleigh_scattering_texture and delta_mie_scattering_texture, and
    // either store them or accumulate them in scattering_texture_ and
    // optional_single_mie_scattering_texture_.
    compute_single_scattering.Use();
    compute_single_scattering.BindImage("delta_rayleigh_scattering_texture", delta_rayleigh_scattering_texture, 0, GL_TEXTURE_3D);
    compute_single_scattering.BindImage("delta_mie_scattering_texture", delta_mie_scattering_texture, 1, GL_TEXTURE_3D);
    compute_single_scattering.BindImage("scattering_texture", scattering_texture, 2, GL_TEXTURE_3D);
    compute_single_scattering.BindImage("single_mie_scattering_texture", single_mie_scattering_texture, 3, GL_TEXTURE_3D);
    compute_single_scattering.BindMat3("luminance_from_radiance", luminance_from_radiance);
    compute_single_scattering.BindTexture2d("transmittance_texture", transmittance_texture, 0);
    compute_single_scattering.BindInt("blend", blend ? 1 : 0);
    dispatch(SCATTERING_TEXTURE_WIDTH, SCATTERING_TEXTURE_HEIGHT, SCATTERING_TEXTURE_DEPTH);
    glMemoryBarrier(GL_ALL_BARRIER_BITS);

    // Compute the 2nd, 3rd and 4th order of scattering, in sequence.
    for (unsigned int scattering_order = 2; scattering_order <= num_scattering_orders; ++scattering_order)
    {
        // Compute the scattering density, and store it in
        // delta_scattering_density_texture.
        compute_scattering_density.Use();
        compute_scattering_density.BindImage("delta_scattering_density_texture", delta_scattering_density_texture, 0, GL_TEXTURE_3D);
        compute_scattering_density.BindTexture2d("transmittance_texture", transmittance_texture, 0);
        compute_scattering_density.BindTexture3d("single_rayleigh_scattering_texture", delta_rayleigh_scattering_texture, 1);
        compute_scattering_density.BindTexture3d("single_mie_scattering_texture", delta_mie_scattering_texture, 2);
        compute_scattering_density.BindTexture3d("multiple_scattering_texture", delta_multiple_scattering_texture, 3);
        compute_scattering_density.BindTexture2d("irradiance_texture", delta_irradiance_texture, 4);
        compute_scattering_density.BindInt("scattering_order", scattering_order);
        dispatch(SCATTERING_TEXTURE_WIDTH, SCATTERING_TEXTURE_HEIGHT, SCATTERING_TEXTURE_DEPTH);
        glMemoryBarrier(GL_ALL_BARRIER_BITS);

        // Compute the indirect irradiance, store it in delta_irradiance_texture and
        // accumulate it in irradiance_texture_.
        compute_indirect_irradiance.Use();
        compute_indirect_irradiance.BindImage("delta_irradiance_texture", delta_irradiance_texture, 0, GL_TEXTURE_2D);
        compute_indirect_irradiance.BindImage("irradiance_texture", irradiance_texture, 1, GL_TEXTURE_2D);
        compute_indirect_irradiance.BindTexture3d("single_rayleigh_scattering_texture", delta_rayleigh_scattering_texture, 0);
        compute_indirect_irradiance.BindTexture3d("single_mie_scattering_texture", delta_mie_scattering_texture, 1);
        compute_indirect_irradiance.BindTexture3d("multiple_scattering_texture", delta_multiple_scattering_texture, 2);
        compute_indirect_irradiance.BindMat3("luminance_from_radiance", luminance_from_radiance);
        compute_indirect_irradiance.BindInt("scattering_order", scattering_order - 1);
        dispatch(IRRADIANCE_TEXTURE_WIDTH, IRRADIANCE_TEXTURE_HEIGHT);
        glMemoryBarrier(GL_ALL_BARRIER_BITS);

        // Compute the multiple scattering, store it in
        // delta_multiple_scattering_texture, and accumulate it in
        // scattering_texture_.
        compute_multiple_scattering.Use();
        compute_multiple_scattering.BindImage("delta_multiple_scattering_texture", delta_multiple_scattering_texture, 0, GL_TEXTURE_3D);
        compute_multiple_scattering.BindImage("scattering_texture", scattering_texture, 1, GL_TEXTURE_3D);
        compute_multiple_scattering.BindTexture2d("transmittance_texture", transmittance_texture, 0);
        compute_multiple_scattering.BindTexture3d("scattering_density_texture", delta_scattering_density_texture, 1);
        compute_multiple_scattering.BindMat3("luminance_from_radiance", luminance_from_radiance);
        dispatch(SCATTERING_TEXTURE_WIDTH, SCATTERING_TEXTURE_HEIGHT, SCATTERING_TEXTURE_DEPTH);
        glMemoryBarrier(GL_ALL_BARRIER_BITS);
    }
}

constexpr GLenum Model::format()
{
    return GL_RGBA;
}

constexpr GLenum Model::internalFormat()
{
    return GL_RGBA32F;
}

}  // namespace bruneton
}  // namespace unisim
