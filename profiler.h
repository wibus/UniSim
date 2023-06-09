#ifndef PROFILER_H
#define PROFILER_H

#include <string>
#include <vector>


namespace unisim
{

typedef unsigned int ProfileIdCpu;
typedef unsigned int ProfileIdGpu;


struct CPUProfilePoint;
struct GPUProfilePoint;

class Profiler
{
    Profiler();
public:
    ~Profiler();

    static Profiler& GetInstance();

    ProfileIdCpu registerCpuPoint(const std::string& name);
    ProfileIdGpu registerGpuPoint(const std::string& name);

    void initize();
    void swapFrames();

    void startCpuPoint(ProfileIdCpu id);
    void stopCpuPoint(ProfileIdCpu id);

    void startGpuPoint(ProfileIdCpu id);
    void stopGpuPoint(ProfileIdCpu id);

    void renderUi();

private:
    bool _initialized;
    unsigned int _gpuFrame;

    std::vector<CPUProfilePoint> _cpuPoints;
    std::vector<GPUProfilePoint> _gpuPoints;
};


struct ScoppedCpuPoint
{
    ScoppedCpuPoint(ProfileIdCpu id) : _id(id)
    {
        Profiler::GetInstance().startCpuPoint(_id);
    }

    ~ScoppedCpuPoint()
    {
        Profiler::GetInstance().stopCpuPoint(_id);
    }

private:
    ProfileIdCpu _id;
};


struct ScoppedGpuPoint
{
    ScoppedGpuPoint(ProfileIdGpu id) : _id(id)
    {
        Profiler::GetInstance().startGpuPoint(_id);
    }

    ~ScoppedGpuPoint()
    {
        Profiler::GetInstance().stopGpuPoint(_id);
    }

private:
    ProfileIdGpu _id;
};

#define DeclareProfilePoint(name) static ProfileIdCpu ProfileIdCpu_##name = Profiler::GetInstance().registerCpuPoint(#name)
#define Profile(name) ScoppedCpuPoint profilePoint_##name(ProfileIdCpu_##name)

#define DeclareProfilePointGpu(name) static ProfileIdGpu ProfileIdGpu_##name = Profiler::GetInstance().registerGpuPoint(#name)
#define ProfileGpu(name) ScoppedGpuPoint profilePointGpu_##name(ProfileIdGpu_##name)

}

#endif // PROFILER_H
