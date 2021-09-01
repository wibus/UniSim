#include "material.h"

#include <stdio.h>
#include <setjmp.h>
#include <jpeglib.h>


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

    _albedo = loadJpeg(fileName);
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

}
