#include "material.h"

#include <iostream>

#include <stdio.h>
#include <setjmp.h>
#include <cstring>
#include <jpeglib.h>
#include <png.h>
#include <tinyexr.h>
#include <functional>
#include <string_view>

#include <imgui/imgui.h>

#include "scene.h"
#include "primitive.h"
#include "instance.h"
#include "resource.h"
#include "profiler.h"


namespace unisim
{

DefineProfilePoint(Material);

DefineResource(MaterialDatabase);
DefineResource(BindlessTextures);


struct GpuMaterial
{
    glm::vec4 albedo;
    glm::vec4 emission;
    glm::vec4 specular;

    int albedoTexture;
    int specularTexture;
    int pad1;
    int pad2;
};

struct ErrorManager
{
    jpeg_error_mgr defaultErrorManager;
    jmp_buf jumpBuffer;
};

void ErrorExit(j_common_ptr cinfo)
{
    ErrorManager* pErrorManager = (ErrorManager*) cinfo->err;
    (*cinfo->err->output_message)(cinfo); // print error message
    longjmp(pErrorManager->jumpBuffer, 1);
}

void OutputMessage(j_common_ptr cinfo)
{
    char buffer[JMSG_LENGTH_MAX];
#include <functional>
#include <string_view>
    (*cinfo->err->format_message) (cinfo, buffer);
    fprintf(stderr, "%s\n", buffer);
}

unsigned char BLACK_UNORM8_DATA[4] = {0, 0, 0, 0};
const Texture Texture::BLACK_UNORM8 = Texture(Texture::UNORM8, BLACK_UNORM8_DATA);

float BLACK_Float32_DATA[4] = {0.0f, 0.0f, 0.0f, 0.0f};
const Texture Texture::BLACK_Float32= Texture(Texture::Float32, (unsigned char*)BLACK_Float32_DATA);


Texture::Texture() :
    width(0),
    height(0),
    format(UNORM8),
    numComponents(4),
    handle(0)
{

}

Texture::~Texture()
{
}

Texture::Texture(Format format, unsigned char* pixelData) :
    width(1),
    height(1),
    format(format),
    numComponents(4),
    handle(0)
{
    data.resize(4);
    data[0] = pixelData[0];
    data[1] = pixelData[1];
    data[2] = pixelData[2];
    data[3] = pixelData[3];
}

Texture* Texture::load(const std::string& fileName)
{
    if(fileName.find(".jpg") != std::string::npos)
        return loadJpeg(fileName);
    else if(fileName.find(".png") != std::string::npos)
        return loadPng(fileName);
    else if(fileName.find(".exr") != std::string::npos)
        return loadExr(fileName);
    else
        std::cerr << "Unknow file type: " << fileName << std::endl;

    return nullptr;
}

Texture* Texture::loadJpeg(const std::string& fileName)
{
    jpeg_decompress_struct cinfo;
    ErrorManager errorManager;

    FILE* pFile = fopen(fileName.c_str(), "rb");
    if (!pFile)
        return nullptr;

    Texture* texture = nullptr;

    // set our custom error handler
    cinfo.err = jpeg_std_error(&errorManager.defaultErrorManager);
    errorManager.defaultErrorManager.error_exit = ErrorExit;
    errorManager.defaultErrorManager.output_message = OutputMessage;
    if (setjmp(errorManager.jumpBuffer))
    {
        // We jump here on errorz
        if(texture)
        {
            delete texture;
            texture = nullptr;
        }
        jpeg_destroy_decompress(&cinfo);
        fclose(pFile);
        return nullptr;
    }

    jpeg_create_decompress(&cinfo);
    jpeg_stdio_src(&cinfo, pFile);
    jpeg_read_header(&cinfo, TRUE);
    jpeg_start_decompress(&cinfo);

    texture = new Texture();
    texture->width = cinfo.image_width;
    texture->height = cinfo.image_height;
    texture->format = Texture::UNORM8;
    texture->numComponents = 4;//cinfo.num_components;
    texture->data.resize(texture->width * texture->height * texture->numComponents);

    if(texture->numComponents == cinfo.num_components)
    {
        while(cinfo.output_scanline < cinfo.image_height)
        {
            uint8_t* p = &texture->data.at(cinfo.output_scanline * cinfo.image_width * cinfo.num_components);
            jpeg_read_scanlines(&cinfo, &p, 1);
        }
    }
    else
    {
        uint8_t* scanline = new uint8_t[cinfo.image_width * cinfo.num_components];

        while(cinfo.output_scanline < cinfo.image_height)
        {
            uint8_t* p = &texture->data.at(cinfo.output_scanline * texture->width * texture->numComponents);
            jpeg_read_scanlines(&cinfo, &scanline, 1);

            for(int i = 0; i < texture->width; ++i)
            {
                p[i * texture->numComponents + 0] = scanline[i * cinfo.num_components + 0];
                p[i * texture->numComponents + 1] = scanline[i * cinfo.num_components + 1];
                p[i * texture->numComponents + 2] = scanline[i * cinfo.num_components + 2];

                if(texture->numComponents > 3)
                {
                    if(cinfo.num_components > 3)
                        p[i * texture->numComponents + 3] = scanline[i * cinfo.num_components + 3];
                    else
                        p[i * texture->numComponents + 3] = 255;
                }
            }
        }

        delete [] scanline;
    }

    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);
    fclose(pFile);

    return texture;
}

Texture* Texture::loadPng(const std::string& fileName)
{
    FILE *fp = fopen(fileName.c_str(), "rb");
    if(!fp)
        return nullptr;

    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if(!png)
        return nullptr;

    png_infop info = png_create_info_struct(png);
    if(!info)
        return nullptr;

    if(setjmp(png_jmpbuf(png)))
        return nullptr;

    png_init_io(png, fp);

    png_read_info(png, info);

    Texture* texture = new Texture();
    texture->width      = png_get_image_width(png, info);
    texture->height     = png_get_image_height(png, info);
    texture->format = Texture::UNORM8;
    texture->numComponents = 4;
    texture->data.resize(texture->width * texture->height * texture->numComponents);

    png_byte color_type = png_get_color_type(png, info);
    png_byte bit_depth  = png_get_bit_depth(png, info);

    if(bit_depth == 16)
      png_set_strip_16(png);

    // PNG_COLOR_TYPE_GRAY_ALPHA is always 8 or 16bit depth
    if(color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_palette_to_rgb(png);

    if(color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
        png_set_expand_gray_1_2_4_to_8(png);

    if(png_get_valid(png, info, PNG_INFO_tRNS))
        png_set_tRNS_to_alpha(png);

    // These color_type don't have an alpha channel then fill it with 0xff
    if(color_type == PNG_COLOR_TYPE_RGB ||
       color_type == PNG_COLOR_TYPE_GRAY ||
       color_type == PNG_COLOR_TYPE_PALETTE)
    {
        png_set_filler(png, 0xFF, PNG_FILLER_AFTER);
    }

    if(color_type == PNG_COLOR_TYPE_GRAY ||
       color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
    {
        png_set_gray_to_rgb(png);
    }

    png_read_update_info(png, info);

    png_bytep *row_pointers = (png_bytep*)malloc(sizeof(png_bytep) * texture->height);
    for(int y = 0; y < texture->height; y++) {
        row_pointers[y] = (png_byte*)malloc(png_get_rowbytes(png,info));
    }

    png_read_image(png, row_pointers);

    fclose(fp);

    png_destroy_read_struct(&png, &info, NULL);

    int lineStride = sizeof(char) * texture->numComponents * texture->width;

    for(int y = 0; y < texture->height; ++y)
    {
        unsigned char* p = &texture->data.at(lineStride * y);
        std::memcpy(p, row_pointers[y], lineStride);
    }

    for(int y = 0; y < texture->height; y++)
        free(row_pointers[y]);
    free(row_pointers);

    return texture;
}

Texture* Texture::loadExr(const std::string& fileName)
{
    const char* input = fileName.c_str();

    const char* err = nullptr;
    float* out; // width * height * RGBA
    int width;
    int height;

    int ret = LoadEXR(&out, &width, &height, input, &err);

    if (ret != TINYEXR_SUCCESS)
    {
        if (err)
        {
            std::cerr << "TinyEXR error :" << err << std::endl;
            FreeEXRErrorMessage(err); // release memory of error message.
        }
        return nullptr;
    }
    else
    {
        Texture* texture = new Texture();
        texture->width = width;
        texture->height = height;
        texture->format = Texture::Float32;
        texture->numComponents = 4;

        unsigned int byteCount = sizeof(float) * (unsigned int)(texture->width * texture->height * texture->numComponents);

        texture->data.resize(byteCount);
        memcpy(&texture->data.at(0), out, byteCount);

        free(out); // release memory of image data

        return texture;
    }
}

void Texture::ui()
{
    int dimensions[2] = {width, height};
    ImGui::InputInt2("Dimensions", &dimensions[0], ImGuiInputTextFlags_ReadOnly);
    ImGui::Text("Format %s", format == Texture::UNORM8 ? "UNORM8" : "Float32");
    ImGui::Text("Num Components %d", numComponents);
    uint64_t handle64 = handle;
    ImGui::Image((void*)handle64, ImVec2(512, (512.0f / dimensions[0]) * dimensions[1]));
}

Material::Material(const std::string& name) :
    _name(name),
    _albedo(nullptr),
    _specular(nullptr),
    _defaultAlbedo(1, 1, 1),
    _defaultEmissionColor(0, 0, 0),
    _defaultEmissionLuminance(0),
    _defaultRoughness(1),
    _defaultMetalness(0),
    _defaultReflectance(0.04)
{

}

Material::~Material()
{
    delete _albedo;
}


void Material::setDefaultAlbedo(const glm::vec3& albedo)
{
    _defaultAlbedo = albedo;
}

void Material::setDefaultEmissionColor(const glm::vec3& emissionColor)
{
    _defaultEmissionColor = emissionColor;
}

void Material::setDefaultEmissionLuminance(float emissionLuminance)
{
    _defaultEmissionLuminance = emissionLuminance;
}

void Material::setDefaultRoughness(float roughness)
{
    _defaultRoughness = roughness;
}

void Material::setDefaultMetalness(float metalness)
{
    _defaultMetalness = metalness;
}

void Material::setDefaultReflectance(float reflectance)
{
    _defaultReflectance = reflectance;
}

bool Material::loadAlbedo(const std::string& fileName)
{
    if(_albedo)
    {
        delete _albedo;
        _albedo = nullptr;
    }

    _albedo = Texture::load(fileName);

    return _albedo != nullptr;
}

bool Material::loadSpecular(const std::string &fileName)
{
    if(_specular)
    {
        delete _specular;
        _specular = nullptr;
    }

    _specular = Texture::load(fileName);

    return _specular != nullptr;
}

void Material::ui()
{
    if(_albedo && ImGui::TreeNode("Albedo Texture"))
    {
        _albedo->ui();
        ImGui::TreePop();
    }

    if(_specular && ImGui::TreeNode("Specular Texture"))
    {
        _specular->ui();
        ImGui::TreePop();
    }

    glm::vec3 albedo = _defaultAlbedo;
    if(ImGui::ColorEdit3("Albedo", &albedo[0]))
        setDefaultAlbedo(albedo);

    glm::vec3 emissionColor = _defaultEmissionColor;
    if(ImGui::ColorEdit3("Emission", &emissionColor[0]))
        setDefaultEmissionColor(emissionColor);

    float emissionLuminance = _defaultEmissionLuminance;
    if(ImGui::InputFloat("Luminance", &emissionLuminance))
        setDefaultEmissionLuminance(emissionLuminance);

    float roughness = _defaultRoughness;
    if(ImGui::SliderFloat("Roughness", &roughness, 0, 1))
        setDefaultRoughness(roughness);

    float metalness = _defaultMetalness;
    if(ImGui::SliderFloat("Metalness", &metalness, 0, 1))
        setDefaultMetalness(metalness);

    float reflectance = _defaultReflectance;
    if(ImGui::InputFloat("Reflectance", &reflectance, 0.01f))
    {
        reflectance = glm::clamp(reflectance, 0.0f, 1.0f);
        setDefaultReflectance(reflectance);
    }
}


MaterialDatabase::MaterialDatabase() :
    GraphicTask("Material Registry")
{
}

void MaterialDatabase::registerMaterial(const std::shared_ptr<Material>& material)
{
    bool isRegistered = isMaterialRegistered(material);
    assert(!isRegistered);
    if(isRegistered)
        return;

    assert(_materialIds.find(uint64_t(material.get())) == _materialIds.end());
    _materialIds[uint64_t(material.get())] = MaterialId(_materials.size());
    _materials.push_back(material);
}

unsigned int MaterialDatabase::materialId(const std::shared_ptr<Material>& material) const
{
    auto it = _materialIds.find(uint64_t(material.get()));
    assert(it != _materialIds.end());

    if(it != _materialIds.end())
        return it->second;
    else
        return MaterialId_Invalid;
}

bool MaterialDatabase::isMaterialRegistered(const std::shared_ptr<Material>& material) const
{
    return _materialIds.find(uint64_t(material.get())) != _materialIds.end();
}

void MaterialDatabase::registerDynamicResources(Context& context)
{
    for(const auto& instance : context.scene.instances())
    {
        for(const auto& primitive : instance->primitives())
        {
            const auto& material = primitive->material();
            if(material && !isMaterialRegistered(material))
                registerMaterial(material);
        }
    }

    ResourceManager& resources = context.resources;

    for(std::size_t i = 0; i < _materials.size(); ++i)
    {
        _materialsResourceIds.push_back({
            resources.registerDynamicResource(_materials[i]->name() + "_texture_albedo"),
            resources.registerDynamicResource(_materials[i]->name() + "_texture_specular"),
            resources.registerDynamicResource(_materials[i]->name() + "_bindless_albedo"),
            resources.registerDynamicResource(_materials[i]->name() + "_bindless_specular"),
        });
    }
}

bool MaterialDatabase::definePathTracerModules(Context& context)
{
    return true;
}

bool MaterialDatabase::defineResources(Context& context)
{
    bool ok = true;

    ResourceManager& resources = context.resources;

    for(std::size_t i = 0; i < _materials.size(); ++i)
    {
        const Material& material = *_materials[i];

        if(material.albedo() != nullptr)
        {
            ok = ok && resources.define<GpuTextureResource>(_materialsResourceIds[i].textureAlbedo, {*material.albedo()});
            GLuint albedoTexId = resources.get<GpuTextureResource>(_materialsResourceIds[i].textureAlbedo).texId;
            ok = ok && resources.define<GpuBindlessResource>(_materialsResourceIds[i].bindlessAlbedo, {material.albedo(), albedoTexId});
        }

        if(material.specular() != nullptr)
        {
            ok = ok && resources.define<GpuTextureResource>(_materialsResourceIds[i].textureSpecular, {*material.specular()});
            GLuint specularTexId = resources.get<GpuTextureResource>(_materialsResourceIds[i].textureSpecular).texId;
            ok = ok && resources.define<GpuBindlessResource>(_materialsResourceIds[i].bindlessSpecular, {material.specular(), specularTexId});
        }
    }

    std::vector<GpuMaterial> gpuMaterials;
    std::vector<GPUBindlessTexture> gpuTextures;
    _hash = toGpu(context, gpuTextures, gpuMaterials);

    ok = ok && context.resources.define<GpuStorageResource>(
        ResourceName(MaterialDatabase),
        {sizeof (GpuMaterial), gpuMaterials.size(), gpuMaterials.data()});

    ok = ok && context.resources.define<GpuStorageResource>(
        ResourceName(BindlessTextures),
        {sizeof (GPUBindlessTexture), gpuTextures.size(), gpuTextures.data()});

    return ok;
}

void MaterialDatabase::setPathTracerResources(
    Context& context,
        PathTracerInterface& interface) const
{
    ResourceManager& resources = context.resources;

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, interface.getSsboBindPoint("Textures"),
                     resources.get<GpuStorageResource>(ResourceName(BindlessTextures)).bufferId);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, interface.getSsboBindPoint("Materials"),
                     resources.get<GpuStorageResource>(ResourceName(MaterialDatabase)).bufferId);
}

void MaterialDatabase::update(Context& context)
{
    Profile(Material);

    std::vector<GpuMaterial> gpuMaterials;
    std::vector<GPUBindlessTexture> gpuTextures;
    uint64_t hash = toGpu(context, gpuTextures, gpuMaterials);

    if(_hash == hash)
        return;

    _hash = hash;

    ResourceManager& resources = context.resources;
    resources.get<GpuStorageResource>(ResourceName(BindlessTextures)).update(
                {sizeof(GPUBindlessTexture), gpuTextures.size(), gpuTextures.data()});
    resources.get<GpuStorageResource>(ResourceName(MaterialDatabase)).update(
                {sizeof(GpuMaterial), gpuMaterials.size(), gpuMaterials.data()});
}

void MaterialDatabase::render(Context& context)
{
}

uint64_t MaterialDatabase::toGpu(
    const Context& context,
        std::vector<GPUBindlessTexture>& gpuBindless,
        std::vector<GpuMaterial>& gpuMaterials)
{
    ResourceManager& resources = context.resources;

    for(std::size_t i = 0; i < _materials.size(); ++i)
    {
        const Material& material = *_materials[i];

        GpuMaterial gpuMaterial;

        gpuMaterial.albedo = glm::vec4(material.defaultAlbedo(), 1.0);

        gpuMaterial.emission = glm::vec4(material.defaultEmissionColor()
                                         * material.defaultEmissionLuminance(),
                                         1.0);

        gpuMaterial.specular = glm::vec4(
                    // Roughness to GGX's 'a' parameter
                    material.defaultRoughness() * material.defaultRoughness(),
                    material.defaultMetalness(),
                    material.defaultReflectance(),
                    0);

        if(material.albedo() != nullptr)
        {
            gpuMaterial.albedoTexture = gpuBindless.size();
            gpuBindless.emplace_back(resources.get<GpuBindlessResource>(_materialsResourceIds[i].bindlessAlbedo).handle);
        }
        else
        {
            gpuMaterial.albedoTexture = -1;
        }

        if(material.specular() != nullptr)
        {
            gpuMaterial.specularTexture = gpuBindless.size();
            gpuBindless.emplace_back(resources.get<GpuBindlessResource>(_materialsResourceIds[i].bindlessSpecular).handle);
        }
        else
        {
            gpuMaterial.specularTexture = -1;
        }

        gpuMaterials.push_back(gpuMaterial);
    }

    uint64_t hash = 0;
    hash = hashVec(gpuBindless, hash);
    hash = hashVec(gpuMaterials, hash);

    return hash;
}


}
