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

DefineResource(BrunetonDirectIrradianceParams);
DefineResource(BrunetonSingleScatteringParams);
DefineResource(BrunetonScatteringDensityParams);
DefineResource(BrunetonIndirectIrradianceParams);
DefineResource(BrunetonMultipleScatteringParams);

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

struct GpuDirectIrradianceParams
{
    glm::int32 blend;
};

const char kComputeDirectIrradianceShader[] =
    R"(
    layout (binding = 0, std140) uniform DirectIrradianceParams
    {
        int blend;
    };

    uniform sampler2D transmittance_texture;

    layout(binding = 0, rgba32f) uniform image2D delta_irradiance_texture;
    layout(binding = 1, rgba32f) uniform image2D irradiance_texture;

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

struct GpuSingleScatteringParams
{
    glm::mat4 luminance_from_radiance;
    glm::int32 blend;
};

const char kComputeSingleScatteringShader[] =
    R"(
    layout (binding = 0, std140) uniform SingleScatteringParams
    {
        mat4 luminance_from_radiance;
        int blend;
    };

    uniform sampler2D transmittance_texture;

    layout(binding = 0, rgba32f) uniform image3D delta_rayleigh_scattering_texture;
    layout(binding = 1, rgba32f) uniform image3D delta_mie_scattering_texture;
    layout(binding = 2, rgba32f) uniform image3D scattering_texture;
    layout(binding = 3, rgba32f) uniform image3D single_mie_scattering_texture;

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

            vec3 scattering = mat3(luminance_from_radiance) * delta_rayleigh.rgb;
            if (blend == 1)
            {
                scattering += imageLoad(scattering_texture, threadID).rgb;
            }
            imageStore(scattering_texture, threadID, vec4(scattering, 0.0));

            vec3 single_mie_scattering = mat3(luminance_from_radiance) * delta_mie;
            if (blend == 1)
            {
                single_mie_scattering += imageLoad(single_mie_scattering_texture, threadID).rgb;
            }
            imageStore(single_mie_scattering_texture, threadID, vec4(single_mie_scattering, 0.0));
        }
    }
)";

struct GpuScatteringDensityParams
{
    glm::int32 scattering_order;
};

const char kComputeScatteringDensityShader[] =
    R"(
    layout (binding = 0, std140) uniform ScatteringDensityParams
    {
        int scattering_order;
    };

    uniform sampler2D transmittance_texture;
    uniform sampler3D single_rayleigh_scattering_texture;
    uniform sampler3D single_mie_scattering_texture;
    uniform sampler3D multiple_scattering_texture;
    uniform sampler2D delta_irradiance_texture;

    layout(binding = 0, rgba32f) uniform image3D delta_scattering_density_texture;

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
                delta_irradiance_texture, fragCoord,
                scattering_order);

            imageStore(delta_scattering_density_texture, threadID, vec4(scattering_density, 0.0));
        }
    }
)";

struct GpuIndirectIrradianceParams
{
    glm::mat4 luminance_from_radiance;
    glm::int32 scattering_order;
};

const char kComputeIndirectIrradianceShader[] =
    R"(
    layout (binding = 0, std140) uniform IndirectIrradianceParams
    {
        mat4 luminance_from_radiance;
        int scattering_order;
    };

    uniform sampler3D single_rayleigh_scattering_texture;
    uniform sampler3D single_mie_scattering_texture;
    uniform sampler3D multiple_scattering_texture;

    layout(binding = 0, rgba32f) uniform image2D delta_irradiance_texture;
    layout(binding = 1, rgba32f) uniform image2D irradiance_texture;

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

            vec3 irradiance = mat3(luminance_from_radiance) * delta_irradiance;
            irradiance += imageLoad(irradiance_texture, threadID).rgb;
            imageStore(irradiance_texture, threadID, vec4(irradiance, 0.0));
        }
    }
)";

struct GpuMultipleScatteringParams
{
    glm::mat4 luminance_from_radiance;
};

const char kComputeMultipleScatteringShader[] =
    R"(
    layout (binding = 0, std140) uniform MultipleScatteringParams
    {
        mat4 luminance_from_radiance;
    };

    uniform sampler2D transmittance_texture;
    uniform sampler3D scattering_density_texture;

    layout(binding = 0, rgba32f) uniform image3D delta_multiple_scattering_texture;
    layout(binding = 1, rgba32f) uniform image3D scattering_texture;

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

            vec4 scattering = vec4(mat3(luminance_from_radiance) * delta_multiple_scattering.rgb / RayleighPhaseFunction(nu), 0.0);
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
    , is_dirty_(true)
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

    is_dirty_ = true;
}


/*
<p>The destructor is trivial:
*/

Model::~Model()
{
}

void Model::registerDynamicResources(GraphicContext& context)
{
}

bool Model::defineResources(GraphicContext& context)
{
    GpuResourceManager& resources = context.resources;

    bool ok = true;

    ok = ok && resources.define<GpuImageResource>(ResourceName(BrunetonTransmittance),
    {
        .width  = TRANSMITTANCE_TEXTURE_WIDTH,
        .height = TRANSMITTANCE_TEXTURE_HEIGHT,
        .depth  = 1,
        .format = Model::internalFormat()
    });
    ok = ok && resources.define<GpuImageResource>(ResourceName(BrunetonScattering),
    {
        .width  = SCATTERING_TEXTURE_WIDTH,
        .height = SCATTERING_TEXTURE_HEIGHT,
        .depth  = SCATTERING_TEXTURE_DEPTH,
        .format = Model::internalFormat()
    });
    ok = ok && resources.define<GpuImageResource>(ResourceName(BrunetonSingleMieScattering),
    {
        .width  = SCATTERING_TEXTURE_WIDTH,
        .height = SCATTERING_TEXTURE_HEIGHT,
        .depth  = SCATTERING_TEXTURE_DEPTH,
        .format = Model::internalFormat()
    });
    ok = ok && resources.define<GpuImageResource>(ResourceName(BrunetonIrradiance),
    {
        .width  = IRRADIANCE_TEXTURE_WIDTH,
        .height = IRRADIANCE_TEXTURE_HEIGHT,
        .depth  = 1,
        .format = Model::internalFormat()
    });

    ok = ok && resources.define<GpuImageResource>(ResourceName(BrunetonDeltaIrradiance),
    {
        .width  = IRRADIANCE_TEXTURE_WIDTH,
        .height = IRRADIANCE_TEXTURE_HEIGHT,
        .depth  = 1,
        .format = Model::internalFormat()
    });
    ok = ok && resources.define<GpuImageResource>(ResourceName(BrunetonDeltaRayleighScattering),
    {
        .width  = SCATTERING_TEXTURE_WIDTH,
        .height = SCATTERING_TEXTURE_HEIGHT,
        .depth  = SCATTERING_TEXTURE_DEPTH,
        .format = Model::internalFormat()
    });
    ok = ok && resources.define<GpuImageResource>(ResourceName(BrunetonDeltaMieScattering),
    {
        .width  = SCATTERING_TEXTURE_WIDTH,
        .height = SCATTERING_TEXTURE_HEIGHT,
        .depth  = SCATTERING_TEXTURE_DEPTH,
        .format = Model::internalFormat()
    });
    ok = ok && resources.define<GpuImageResource>(ResourceName(BrunetonDeltaScatteringDensity),
    {
        .width  = SCATTERING_TEXTURE_WIDTH,
        .height = SCATTERING_TEXTURE_HEIGHT,
        .depth  = SCATTERING_TEXTURE_DEPTH,
        .format = Model::internalFormat()
    });

    resources.define<GpuConstantResource>(ResourceName(BrunetonDirectIrradianceParams)  , {sizeof(GpuDirectIrradianceParams),   {}});
    resources.define<GpuConstantResource>(ResourceName(BrunetonSingleScatteringParams)  , {sizeof(GpuSingleScatteringParams),   {}});
    resources.define<GpuConstantResource>(ResourceName(BrunetonScatteringDensityParams) , {sizeof(GpuScatteringDensityParams),  {}});
    resources.define<GpuConstantResource>(ResourceName(BrunetonIndirectIrradianceParams), {sizeof(GpuIndirectIrradianceParams), {}});
    resources.define<GpuConstantResource>(ResourceName(BrunetonMultipleScatteringParams), {sizeof(GpuMultipleScatteringParams), {}});

    return ok;
}

bool Model::defineShaders(GraphicContext& context)
{
    GpuResourceManager& resources = context.resources;

    for(std::size_t i = 0; i < precompute_passes_.size(); ++i)
    {
        auto& pass = precompute_passes_[i];
        std::string suffix = "_" + std::to_string(i);
        std::string header = glsl_header_factory_(pass.lambdas, false);

        {
            pass.transmittance.interface.reset(new GpuProgramInterface());
            pass.transmittance.interface->declareImage({.name = "transmittance_image"});
            pass.transmittance.program.reset();

            GraphicShaderPtr transmittanceShader;
            if (!generateShader(transmittanceShader, ShaderType::Compute, "Transmittance"+suffix, {header + kComputeTransmittanceShader}, {}))
                return false;
            if (!generateComputeProgram(pass.transmittance.program, "Transmittance"+suffix, {transmittanceShader}))
                return false;
        }

        {
            pass.direct_irradiance.interface.reset(new GpuProgramInterface());
            pass.direct_irradiance.interface->declareConstant({.name = "DirectIrradianceParams"});
            pass.direct_irradiance.interface->declareTexture({.name = "transmittance_texture"});
            pass.direct_irradiance.interface->declareImage({.name = "delta_irradiance_texture"});
            pass.direct_irradiance.interface->declareImage({.name = "irradiance_texture"});
            pass.direct_irradiance.program.reset();

            GraphicShaderPtr directIrradianceShader;
            if (!generateShader(directIrradianceShader, ShaderType::Compute, "DirectIrradiance"+suffix, {header + kComputeDirectIrradianceShader}, {}))
                return false;
            if (!generateComputeProgram(pass.direct_irradiance.program, "DirectIrradiance"+suffix, {directIrradianceShader}))
                return false;
        }

        {
            pass.single_scattering.interface.reset(new GpuProgramInterface());
            pass.single_scattering.interface->declareConstant({.name = "SingleScatteringParams"});
            pass.single_scattering.interface->declareTexture({.name = "transmittance_texture"});
            pass.single_scattering.interface->declareImage({.name = "delta_rayleigh_scattering_texture"});
            pass.single_scattering.interface->declareImage({.name = "delta_mie_scattering_texture"});
            pass.single_scattering.interface->declareImage({.name = "scattering_texture"});
            pass.single_scattering.interface->declareImage({.name = "single_mie_scattering_texture"});
            pass.single_scattering.program.reset();

            GraphicShaderPtr singleScatteringShader;
            if (!generateShader(singleScatteringShader, ShaderType::Compute, "SingleScattering"+suffix, {header + kComputeSingleScatteringShader}, {}))
                return false;
            if (!generateComputeProgram(pass.single_scattering.program, "SingleScattering"+suffix, {singleScatteringShader}))
                return false;
        }

        {
            pass.scattering_density.interface.reset(new GpuProgramInterface());
            pass.scattering_density.interface->declareConstant({.name = "ScatteringDensityParams"});
            pass.scattering_density.interface->declareTexture({.name = "transmittance_texture"});
            pass.scattering_density.interface->declareTexture({.name = "single_rayleigh_scattering_texture"});
            pass.scattering_density.interface->declareTexture({.name = "single_mie_scattering_texture"});
            pass.scattering_density.interface->declareTexture({.name = "multiple_scattering_texture"});
            pass.scattering_density.interface->declareTexture({.name = "delta_irradiance_texture"});
            pass.scattering_density.interface->declareImage({.name = "delta_scattering_density_texture"});
            pass.scattering_density.program.reset();

            GraphicShaderPtr scatteringDensityShader;
            if (!generateShader(scatteringDensityShader, ShaderType::Compute, "ScatteringDensity"+suffix, {header + kComputeScatteringDensityShader}, {}))
                return false;
            if (!generateComputeProgram(pass.scattering_density.program, "ScatteringDensity"+suffix, {scatteringDensityShader}))
                return false;
        }

        {
            pass.indirect_irradiance.interface.reset(new GpuProgramInterface());
            pass.indirect_irradiance.interface->declareConstant({.name = "IndirectIrradianceParams"});
            pass.indirect_irradiance.interface->declareTexture({.name = "single_rayleigh_scattering_texture"});
            pass.indirect_irradiance.interface->declareTexture({.name = "single_mie_scattering_texture"});
            pass.indirect_irradiance.interface->declareTexture({.name = "multiple_scattering_texture"});
            pass.indirect_irradiance.interface->declareImage({.name = "delta_irradiance_texture"});
            pass.indirect_irradiance.interface->declareImage({.name = "irradiance_texture"});
            pass.indirect_irradiance.program.reset();

            GraphicShaderPtr indirectIrradianceShader;
            if (!generateShader(indirectIrradianceShader, ShaderType::Compute, "IndirectIrradiance"+suffix, {header + kComputeIndirectIrradianceShader}, {}))
                return false;
            if (!generateComputeProgram(pass.indirect_irradiance.program, "IndirectIrradiance"+suffix, {indirectIrradianceShader}))
                return false;
        }

        {
            pass.multiple_scattering.interface.reset(new GpuProgramInterface());
            pass.multiple_scattering.interface->declareConstant({.name = "MultipleScatteringParams"});
            pass.multiple_scattering.interface->declareTexture({.name = "transmittance_texture"});
            pass.multiple_scattering.interface->declareTexture({.name = "scattering_density_texture"});
            pass.multiple_scattering.interface->declareImage({.name = "delta_multiple_scattering_texture"});
            pass.multiple_scattering.interface->declareImage({.name = "scattering_texture"});
            pass.multiple_scattering.program.reset();

            GraphicShaderPtr multipleScatteringShader;
            if (!generateShader(multipleScatteringShader, ShaderType::Compute, "MultipleScattering"+suffix, {header + kComputeMultipleScatteringShader}, {}))
                return false;
            if (!generateComputeProgram(pass.multiple_scattering.program, "MultipleScattering"+suffix, {multipleScatteringShader}))
                return false;
        }
    }

    if (final_compute_transmittance_)
    {
        std::string header = glsl_header_factory_({kLambdaR, kLambdaG, kLambdaB}, false);

        final_compute_transmittance_->interface.reset(new GpuProgramInterface());
        final_compute_transmittance_->interface->declareImage({.name = "transmittance_image"});
        final_compute_transmittance_->program.reset();

        GraphicShaderPtr transmittanceShader;
        if (!generateShader(transmittanceShader, ShaderType::Compute, "Transmittance_final", {header + kComputeTransmittanceShader}, {}))
            return false;
        if (!generateComputeProgram(final_compute_transmittance_->program, "Transmittance_final", {transmittanceShader}))
            return false;
    }

    return true;
}

bool Model::definePathTracerModules(
    GraphicContext& context,
    std::vector<std::shared_ptr<PathTracerModule>>& modules)
{
    // Create and compile the shader providing our API.
    std::string shaderSrc = glsl_header_factory_({kLambdaR, kLambdaG, kLambdaB}, false)
                            + (precomputeIlluminance() ? "" : "#define RADIANCE_API_ENABLED\n")
                            + kAtmosphereShader;

    GraphicShaderPtr skyModelShader;
    generateShader(skyModelShader, ShaderType::Compute, "Sky Model", {shaderSrc}, {});

    if (!skyModelShader)
        return false;

    std::shared_ptr<PathTracerModule> modelModule(new PathTracerModule("Sky Model", skyModelShader));
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
    if (is_dirty_)
    {
        Recompute(context);
        is_dirty_ = false;
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
    double sky_k_r, sky_k_g, sky_k_b;
    if (precomputeIlluminance())
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
    glsl_header_factory_ = [=](const vec3& lambdas, bool version)
    {
        return std::string(version ? "#version 440\n" : "") +
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

    if (num_precomputed_wavelengths_ <= 3)
    {
        vec3 lambdas{kLambdaR, kLambdaG, kLambdaB};
        mat3 luminance_from_radiance{1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0};
        precompute_passes_.push_back({
            .luminance_from_radiance = luminance_from_radiance,
            .lambdas = lambdas,
            .blend = false
        });
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

            precompute_passes_.push_back({
                .luminance_from_radiance = luminance_from_radiance,
                .lambdas = lambdas,
                .blend = (i > 0)
            });
        }

        final_compute_transmittance_.reset(new PrecomputeSubPass());
    }
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

    // The precomputations require temporary textures, in particular to store the
    // contribution of one scattering order, which is needed to compute the next
    // order of scattering (the final precomputed textures store the sum of all
    // the scattering orders). We allocate them here, and destroy them at the end
    // of this method.

    // delta_multiple_scattering_texture is only needed to compute scattering
    // order 3 or more, while delta_rayleigh_scattering_texture and
    // delta_mie_scattering_texture are only needed to compute double scattering.
    // Therefore, to save memory, we can store delta_rayleigh_scattering_texture
    // and delta_multiple_scattering_texture in the same GPU texture.

    // The actual precomputations depend on whether we want to store precomputed
    // irradiance or illuminance values.
    for(std::size_t i = 0; i < precompute_passes_.size(); ++i)
    {
        auto& pass = precompute_passes_[i];
        Precompute(
            context,
            pass,
            num_scattering_orders);
    }

    if (final_compute_transmittance_)
    {
        // After the above iterations, the transmittance texture contains the
        // transmittance for the 3 wavelengths used at the last iteration. But we
        // want the transmittance at kLambdaR, kLambdaG, kLambdaB instead, so we
        // must recompute it here for these 3 wavelengths:
        CompiledGpuProgramInterface compiledGpi;
        if (!final_compute_transmittance_->interface->compile(compiledGpi, *final_compute_transmittance_->program))
            return;

        GraphicProgramScope programScope(*final_compute_transmittance_->program);

        context.device.bindImage(resources.get<GpuImageResource>(ResourceName(BrunetonTransmittance)),
                                 compiledGpi.getImageBindPoint("transmittance_image"));

        context.device.dispatch(TRANSMITTANCE_TEXTURE_WIDTH, TRANSMITTANCE_TEXTURE_HEIGHT);
    }

    assert(glGetError() == 0);
}

/*
<p>Finally, we provide the actual implementation of the precomputation algorithm
described in Algorithm 4.1 of
<a href="https://hal.inria.fr/inria-00288758/en">our paper</a>. Each step is
explained by the inline comments below.
*/
void Model::Precompute(
    GraphicContext& context,
    const PrecomputePass& pass,
    unsigned int num_scattering_orders)
{
    glm::mat4 gpuLuminanceFromRadiance = glm::mat4(
        pass.luminance_from_radiance[0], pass.luminance_from_radiance[3], pass.luminance_from_radiance[6], 0,
        pass.luminance_from_radiance[1], pass.luminance_from_radiance[4], pass.luminance_from_radiance[7], 0,
        pass.luminance_from_radiance[2], pass.luminance_from_radiance[5], pass.luminance_from_radiance[8], 0,
        0, 0, 0, 0);

    GpuResourceManager& resources = context.resources;

    {
        // Compute the transmittance, and store it in transmittance_texture_.
        CompiledGpuProgramInterface compiledGpi;
        if (!pass.transmittance.interface->compile(compiledGpi, *pass.transmittance.program))
            return;

        GraphicProgramScope programScope(*pass.transmittance.program);

        context.device.bindImage(resources.get<GpuImageResource>(ResourceName(BrunetonTransmittance)),
                                 compiledGpi.getImageBindPoint("transmittance_image"));

        context.device.dispatch(TRANSMITTANCE_TEXTURE_WIDTH, TRANSMITTANCE_TEXTURE_HEIGHT);
    }

    {
        // Compute the direct irradiance, store it in delta_irradiance_texture and,
        // depending on 'blend', either initialize irradiance_texture_ with zeros or
        // leave it unchanged (we don't want the direct irradiance in
        // irradiance_texture_, but only the irradiance from the sky).
        GpuDirectIrradianceParams directIrradianceParams =
            {
                .blend = pass.blend ? 1 : 0
            };
        resources.update<GpuConstantResource>(ResourceName(BrunetonDirectIrradianceParams),
                                              {
                                                  .size = sizeof(directIrradianceParams),
                                                  .data = &directIrradianceParams
                                              });

        CompiledGpuProgramInterface compiledGpi;
        if (!pass.direct_irradiance.interface->compile(compiledGpi, *pass.direct_irradiance.program))
            return;

        GraphicProgramScope programScope(*pass.direct_irradiance.program);

        context.device.bindBuffer(resources.get<GpuConstantResource>(ResourceName(BrunetonDirectIrradianceParams)),
                                  compiledGpi.getConstantBindPoint("DirectIrradianceParams"));
        context.device.bindTexture(resources.get<GpuImageResource>(ResourceName(BrunetonTransmittance)),
                                   compiledGpi.getTextureBindPoint("transmittance_texture"));
        context.device.bindImage(resources.get<GpuImageResource>(ResourceName(BrunetonDeltaIrradiance)),
                                 compiledGpi.getImageBindPoint("delta_irradiance_texture"));
        context.device.bindImage(resources.get<GpuImageResource>(ResourceName(BrunetonIrradiance)),
                                 compiledGpi.getImageBindPoint("irradiance_texture"));

        context.device.dispatch(IRRADIANCE_TEXTURE_WIDTH, IRRADIANCE_TEXTURE_WIDTH);
    }

    {
        // Compute the rayleigh and mie single scattering, store them in
        // delta_rayleigh_scattering_texture and delta_mie_scattering_texture, and
        // either store them or accumulate them in scattering_texture_ and
        // optional_single_mie_scattering_texture_.
        GpuSingleScatteringParams singleScatteringParams =
            {
                .luminance_from_radiance = gpuLuminanceFromRadiance,
                .blend = pass.blend ? 1 : 0
            };
        resources.update<GpuConstantResource>(ResourceName(BrunetonSingleScatteringParams),
                                              {
                                                  .size = sizeof(singleScatteringParams),
                                                  .data = &singleScatteringParams
                                              });

        CompiledGpuProgramInterface compiledGpi;
        if (!pass.single_scattering.interface->compile(compiledGpi, *pass.single_scattering.program))
            return;

        GraphicProgramScope programScope(*pass.single_scattering.program);

        context.device.bindBuffer(resources.get<GpuConstantResource>(ResourceName(BrunetonSingleScatteringParams)),
                                  compiledGpi.getConstantBindPoint("SingleScatteringParams"));
        context.device.bindTexture(resources.get<GpuImageResource>(ResourceName(BrunetonTransmittance)),
                                   compiledGpi.getTextureBindPoint("transmittance_texture"));
        context.device.bindImage(resources.get<GpuImageResource>(ResourceName(BrunetonDeltaRayleighScattering)),
                                 compiledGpi.getImageBindPoint("delta_rayleigh_scattering_texture"));
        context.device.bindImage(resources.get<GpuImageResource>(ResourceName(BrunetonDeltaMieScattering)),
                                 compiledGpi.getImageBindPoint("delta_mie_scattering_texture"));
        context.device.bindImage(resources.get<GpuImageResource>(ResourceName(BrunetonScattering)),
                                 compiledGpi.getImageBindPoint("scattering_texture"));
        context.device.bindImage(resources.get<GpuImageResource>(ResourceName(BrunetonSingleMieScattering)),
                                 compiledGpi.getImageBindPoint("single_mie_scattering_texture"));

        context.device.dispatch(SCATTERING_TEXTURE_WIDTH, SCATTERING_TEXTURE_HEIGHT, SCATTERING_TEXTURE_DEPTH);
    }

    // Compute the 2nd, 3rd and 4th order of scattering, in sequence.
    for (unsigned int scattering_order = 2; scattering_order <= num_scattering_orders; ++scattering_order)
    {
        {
            // Compute the scattering density, and store it in
            // delta_scattering_density_texture.
            GpuScatteringDensityParams scatteringDensityParams = {.scattering_order = int(scattering_order)};
            resources.update<GpuConstantResource>(ResourceName(BrunetonScatteringDensityParams),
                                                  {
                                                      .size = sizeof(scatteringDensityParams),
                                                      .data = &scatteringDensityParams
                                                  });

            CompiledGpuProgramInterface compiledGpi;
            if (!pass.scattering_density.interface->compile(compiledGpi, *pass.scattering_density.program))
                return;

            GraphicProgramScope programScope(*pass.scattering_density.program);

            context.device.bindBuffer(resources.get<GpuConstantResource>(ResourceName(BrunetonScatteringDensityParams)),
                                      compiledGpi.getConstantBindPoint("ScatteringDensityParams"));
            context.device.bindTexture(resources.get<GpuImageResource>(ResourceName(BrunetonTransmittance)),
                                       compiledGpi.getTextureBindPoint("transmittance_texture"));
            context.device.bindTexture(resources.get<GpuImageResource>(ResourceName(BrunetonDeltaRayleighScattering)),
                                       compiledGpi.getTextureBindPoint("single_rayleigh_scattering_texture"));
            context.device.bindTexture(resources.get<GpuImageResource>(ResourceName(BrunetonDeltaMieScattering)),
                                       compiledGpi.getTextureBindPoint("single_mie_scattering_texture"));
            context.device.bindTexture(resources.get<GpuImageResource>(ResourceName(BrunetonDeltaRayleighScattering)),
                                       compiledGpi.getTextureBindPoint("multiple_scattering_texture"));
            context.device.bindTexture(resources.get<GpuImageResource>(ResourceName(BrunetonDeltaIrradiance)),
                                       compiledGpi.getTextureBindPoint("delta_irradiance_texture"));
            context.device.bindImage(resources.get<GpuImageResource>(ResourceName(BrunetonDeltaScatteringDensity)),
                                     compiledGpi.getImageBindPoint("delta_scattering_density_texture"));

            context.device.dispatch(SCATTERING_TEXTURE_WIDTH, SCATTERING_TEXTURE_HEIGHT, SCATTERING_TEXTURE_DEPTH);
        }

        {
            // Compute the indirect irradiance, store it in delta_irradiance_texture and
            // accumulate it in irradiance_texture_.
            GpuIndirectIrradianceParams indirectIrradianceParams = {.luminance_from_radiance = gpuLuminanceFromRadiance, .scattering_order = int(scattering_order-1)};
            resources.update<GpuConstantResource>(ResourceName(BrunetonIndirectIrradianceParams),
                                                  {
                                                      .size = sizeof(indirectIrradianceParams),
                                                      .data = &indirectIrradianceParams
                                                  });

            CompiledGpuProgramInterface compiledGpi;
            if (!pass.indirect_irradiance.interface->compile(compiledGpi, *pass.indirect_irradiance.program))
                return;

            GraphicProgramScope programScope(*pass.indirect_irradiance.program);

            context.device.bindBuffer(resources.get<GpuConstantResource>(ResourceName(BrunetonIndirectIrradianceParams)),
                                      compiledGpi.getConstantBindPoint("IndirectIrradianceParams"));
            context.device.bindTexture(resources.get<GpuImageResource>(ResourceName(BrunetonDeltaRayleighScattering)),
                                       compiledGpi.getTextureBindPoint("single_rayleigh_scattering_texture"));
            context.device.bindTexture(resources.get<GpuImageResource>(ResourceName(BrunetonDeltaMieScattering)),
                                       compiledGpi.getTextureBindPoint("single_mie_scattering_texture"));
            context.device.bindTexture(resources.get<GpuImageResource>(ResourceName(BrunetonDeltaRayleighScattering)),
                                       compiledGpi.getTextureBindPoint("multiple_scattering_texture"));
            context.device.bindImage(resources.get<GpuImageResource>(ResourceName(BrunetonDeltaIrradiance)),
                                     compiledGpi.getImageBindPoint("delta_irradiance_texture"));
            context.device.bindImage(resources.get<GpuImageResource>(ResourceName(BrunetonIrradiance)),
                                     compiledGpi.getImageBindPoint("irradiance_texture"));

            context.device.dispatch(IRRADIANCE_TEXTURE_WIDTH, IRRADIANCE_TEXTURE_HEIGHT);
        }

        {
            // Compute the multiple scattering, store it in
            // delta_multiple_scattering_texture, and accumulate it in
            // scattering_texture_.
            GpuMultipleScatteringParams multipleScatteringParams = {.luminance_from_radiance = gpuLuminanceFromRadiance};
            resources.update<GpuConstantResource>(ResourceName(BrunetonMultipleScatteringParams),
                                                  {
                                                      .size = sizeof(multipleScatteringParams),
                                                      .data = &multipleScatteringParams
                                                  });

            CompiledGpuProgramInterface compiledGpi;
            if (!pass.multiple_scattering.interface->compile(compiledGpi, *pass.multiple_scattering.program))
                return;

            GraphicProgramScope programScope(*pass.multiple_scattering.program);

            context.device.bindBuffer(resources.get<GpuConstantResource>(ResourceName(BrunetonMultipleScatteringParams)),
                                      compiledGpi.getConstantBindPoint("MultipleScatteringParams"));
            context.device.bindTexture(resources.get<GpuImageResource>(ResourceName(BrunetonTransmittance)),
                                       compiledGpi.getTextureBindPoint("transmittance_texture"));
            context.device.bindTexture(resources.get<GpuImageResource>(ResourceName(BrunetonDeltaScatteringDensity)),
                                       compiledGpi.getTextureBindPoint("scattering_density_texture"));
            context.device.bindImage(resources.get<GpuImageResource>(ResourceName(BrunetonDeltaRayleighScattering)),
                                     compiledGpi.getImageBindPoint("delta_multiple_scattering_texture"));
            context.device.bindImage(resources.get<GpuImageResource>(ResourceName(BrunetonScattering)),
                                     compiledGpi.getImageBindPoint("scattering_texture"));

            context.device.dispatch(SCATTERING_TEXTURE_WIDTH, SCATTERING_TEXTURE_HEIGHT, SCATTERING_TEXTURE_DEPTH);
        }
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
