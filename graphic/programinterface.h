#ifndef PROGRAMINTERFACE_H
#define PROGRAMINTERFACE_H

#include <map>
#include <set>
#include <memory>
#include <vector>
#include <string>

#include "shader.h"


#ifdef UNISIM_GRAPHIC_BACKEND_VK
namespace pils
{
namespace gpu
{
class DescriptorBinding;
class DescriptorSetLayout;
class PipelineLayout;
class DescriptorSet;
class Fence;
}
}
#endif // UNISIM_GRAPHIC_BACKEND_VK


namespace unisim
{

class GpuDevice;

class GpuConstantResource;
class GpuStorageResource;
class GpuTextureResource;
class GpuImageResource;

class GpuBindlessTextureResourceHandle;
class GpuConstantResourceHandle;
class GpuStorageResourceHandle;
class GpuTextureResourceHandle;
class GpuImageResourceHandle;
class GpuBindlessTexture;

struct ProgramBinding
{
    std::string name;

#ifdef UNISIM_GRAPHIC_BACKEND_GL
    uint32_t bindPoint = -1;
    uint32_t descriptorCount;
#endif // UNISIM_GRAPHIC_BACKEND_GL

#ifdef UNISIM_GRAPHIC_BACKEND_VK
    std::shared_ptr<pils::gpu::DescriptorBinding> binding;
#endif // UNISIM_GRAPHIC_BACKEND_VK
};

class ProgramInterfaceTemplate
{
public:
    ProgramInterfaceTemplate(ProgramType programType);
    ProgramInterfaceTemplate(ProgramType programType, const ProgramInterfaceTemplate& parent);

    ProgramType programType() const { return _programType; }

    void declareBindless(const std::string& name, uint32_t descriptorCount = 1);
    void declareConstant(const std::string& name, uint32_t descriptorCount = 1);
    void declareStorage(const std::string& name, uint32_t descriptorCount = 1);
    void declareTexture(const std::string& name, uint32_t descriptorCount = 1);
    void declareImage(const std::string& name, uint32_t descriptorCount = 1);

    const std::vector<ProgramBinding>& bindless() const { return  _bindless;}
    const std::vector<ProgramBinding>& constants() const { return  _constants;}
    const std::vector<ProgramBinding>& storages() const { return  _storages;}
    const std::vector<ProgramBinding>& textures() const { return  _textures;}
    const std::vector<ProgramBinding>& images() const { return  _images;}

#ifdef UNISIM_GRAPHIC_BACKEND_VK
    uint32_t setIndex() const { return _setIndex; }
    std::shared_ptr<pils::gpu::DescriptorSetLayout> computeSetLayout(const GpuDevice& gpuDevice) const;
    std::shared_ptr<pils::gpu::PipelineLayout> computePipelineLayout(const GpuDevice& gpuDevice) const;
#endif // UNISIM_GRAPHIC_BACKEND_VK

private:
    ProgramType _programType;
    std::vector<ProgramBinding> _bindless;
    std::vector<ProgramBinding> _constants;
    std::vector<ProgramBinding> _storages;
    std::vector<ProgramBinding> _textures;
    std::vector<ProgramBinding> _images;

#ifdef UNISIM_GRAPHIC_BACKEND_GL
    uint32_t _nextBindlessStorageBinding;
    uint32_t _nextConstantBinding;
    uint32_t _nextTextureBinding;
    uint32_t _nextImageBinding;
#endif // UNISIM_GRAPHIC_BACKEND_GL

#ifdef UNISIM_GRAPHIC_BACKEND_VK
    uint32_t _setIndex;
    uint32_t _nextBinding;
    pils::gpu::ShaderStage _shaderStages;

    const ProgramInterfaceTemplate* _parent;

    mutable bool _isSetLayoutDirty;
    mutable std::shared_ptr<pils::gpu::DescriptorSetLayout> _cachedSetLayout;
#endif // UNISIM_GRAPHIC_BACKEND_VK
};


class ProgramInterface
{
public:
    ProgramInterface(const GpuDevice& gpuDevice, const ProgramInterfaceTemplate& piTemplate);
    ~ProgramInterface();

    void setBindless(const std::string& name, const GpuBindlessTexture& resource, uint32_t arrayIndex = 0);
    void setConstant(const std::string& name, const GpuConstantResource& resource, uint32_t arrayIndex = 0);
    void setStorage(const std::string& name, const GpuStorageResource& resource, uint32_t arrayIndex = 0);
    void setTexture(const std::string& name, const GpuTextureResource& resource, uint32_t arrayIndex = 0);
    void setTexture(const std::string& name, const GpuImageResource& resource, uint32_t arrayIndex = 0);
    void setImage(const std::string& name, const GpuImageResource& resource, uint32_t arrayIndex = 0);
    bool compileAndSet(const GpuDevice& gpuDevice, const Program& program);

private:
    ProgramType _programType;

    template<typename T>
    struct BindingData
    {
        ProgramBinding binding;
        std::vector<T> descriptors;
    };

    std::map<std::string, BindingData<GpuBindlessTextureResourceHandle>> _bindlessData;
    std::map<std::string, BindingData<GpuConstantResourceHandle>> _constantData;
    std::map<std::string, BindingData<GpuStorageResourceHandle>> _storageData;
    std::map<std::string, BindingData<GpuTextureResourceHandle>> _textureData;
    std::map<std::string, BindingData<GpuImageResourceHandle>> _imageData;

#ifdef UNISIM_GRAPHIC_BACKEND_GL
    bool setBindlessBindPoint(const Program& program, const ProgramBinding& binding);
    bool setConstantBindPoint(const Program& program, const ProgramBinding& binding);
    bool setStorageBindPoint(const Program& program, const ProgramBinding& binding);
    bool setTextureBindPoint(const Program& program, const ProgramBinding& binding);
    bool setImageBindPoint(const Program& program,  const ProgramBinding& binding);

    struct BindlessBacking
    {
        bool isDirty;
        GLuint bufferId;
    };

    std::map<std::string, BindlessBacking> _bindlessBacking;
#endif // UNISIM_GRAPHIC_BACKEND_GL

#ifdef UNISIM_GRAPHIC_BACKEND_VK
    bool _isCompute;
    uint32_t _setIndex;
    uint32_t _nextBinding;
    std::shared_ptr<pils::gpu::DescriptorSetLayout> _layout;

    void recycleDescriptorSet(const GpuDevice& gpuDevice);

    std::map<std::string, std::set<uint32_t>> _dirtyBindlessDescriptors;

    bool _isDescriptorSetDirty;
    std::shared_ptr<pils::gpu::DescriptorSet> _cachedDescriptorSet;
    std::vector<std::pair<
        std::shared_ptr<pils::gpu::DescriptorSet>,
        std::shared_ptr<pils::gpu::Fence>>> _recycledDesciptorSets;
#endif // UNISIM_GRAPHIC_BACKEND_VK
};

typedef std::shared_ptr<ProgramInterface> GpuProgramInterfacePtr;

}

#endif // PROGRAMINTERFACE_H
