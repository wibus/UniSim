#include "material.h"

#include <iostream>

#include <stdio.h>
#include <setjmp.h>
#include <cstring>
#include <jpeglib.h>
#include <png.h>


namespace unisim
{

Material::Material(const std::string& name) :
    _name(name),
    _albedo(nullptr)
{

}

Material::~Material()
{
    delete _albedo;
}


void Material::setDefaultAlbedo(const glm::dvec3& albedo)
{
    _defaultAlbedo = albedo;
}

void Material::setDefaultEmission(const glm::dvec3& emission)
{
    _defaultEmission = emission;
}

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
    (*cinfo->err->format_message) (cinfo, buffer);
    fprintf(stderr, "%s\n", buffer);
}

bool Material::loadAlbedo(const std::string& fileName)
{
    if(_albedo)
    {
        delete _albedo;
        _albedo = nullptr;
    }

    if(fileName.find(".jpeg") != std::string::npos)
        _albedo = loadJpeg(fileName);
    else if(fileName.find(".png") != std::string::npos)
        _albedo = loadPng(fileName);
    else
        std::cerr << "Unknow file type: " << fileName << std::endl;

    return _albedo != nullptr;
}

Texture* Material::loadJpeg(const std::string& fileName)
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
    texture->numComponents = 4;//cinfo.num_components;
    texture->data = new uint8_t[texture->width * texture->height * texture->numComponents];

    if(texture->numComponents == cinfo.num_components)
    {
        while(cinfo.output_scanline < cinfo.image_height)
        {
            uint8_t* p = texture->data + cinfo.output_scanline * cinfo.image_width * cinfo.num_components;
            jpeg_read_scanlines(&cinfo, &p, 1);
        }
    }
    else
    {
        uint8_t* scanline = new uint8_t[cinfo.image_width * cinfo.num_components];

        while(cinfo.output_scanline < cinfo.image_height)
        {
            uint8_t* p = texture->data + cinfo.output_scanline * texture->width * texture->numComponents;
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
                        p[i * texture->numComponents + 3] = 0;
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

Texture* Material::loadPng(const std::string& fileName)
{
    FILE *fp = fopen(fileName.c_str(), "rb");

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
    texture->numComponents = 4;
    texture->data = new uint8_t[texture->width * texture->height * texture->numComponents];

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
        unsigned char* p = texture->data + lineStride * y;
        std::memcpy(p, row_pointers[y], lineStride);
    }

    for(int y = 0; y < texture->height; y++)
        free(row_pointers[y]);
    free(row_pointers);

    return texture;
}

}
