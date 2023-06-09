#include "profiler.h"

#include <chrono>
#include <cassert>

#include <GL/glew.h>

#include <imgui/imgui.h>


namespace unisim
{

typedef std::chrono::high_resolution_clock CpuClock;
typedef std::chrono::time_point<CpuClock> CpuTime;
typedef std::chrono::nanoseconds CpuDuration;
#define CpuTimeNow() CpuClock::now();

struct CPUProfilePoint
{
    ProfileIdCpu id;
    std::string name;
    CpuTime start;
    CpuTime stop;
    uint64_t elapsedNs;
};


struct GPUProfilePoint
{
    ProfileIdCpu id;
    std::string name;

    // Double buffered
    GLuint queries[2][2];
    uint64_t elapsedNs;
};

Profiler::Profiler() :
    _initialized(false)
{

}

Profiler::~Profiler()
{

}

Profiler& Profiler::GetInstance()
{
    static Profiler profiler;
    return profiler;
}

ProfileIdCpu Profiler::registerCpuPoint(const std::string& name)
{
    assert(!_initialized);

    ProfileIdCpu id = ProfileIdCpu(_cpuPoints.size());

    CPUProfilePoint& pt = _cpuPoints.emplace_back();
    pt.id = id;
    pt.name = name;
    pt.start = CpuTimeNow();
    pt.stop = pt.start;
    pt.elapsedNs = 0;

    return id;
}

ProfileIdGpu Profiler::registerGpuPoint(const std::string& name)
{
    assert(!_initialized);

    ProfileIdGpu id = ProfileIdGpu(_gpuPoints.size());

    GPUProfilePoint& pt = _gpuPoints.emplace_back();
    pt.id = id;
    pt.name = name;
    pt.queries[0][0] = 0;
    pt.queries[0][1] = 0;
    pt.queries[1][0] = 0;
    pt.queries[1][1] = 0;
    pt.elapsedNs = 0;

    return id;
}

void Profiler::initize()
{
    _initialized = true;

    _gpuFrame = 0;

    for(GPUProfilePoint& gpuPt : _gpuPoints)
    {
        glGenQueries(4, &gpuPt.queries[0][0]);
        glQueryCounter(gpuPt.queries[_gpuFrame][0], GL_TIMESTAMP);
        glQueryCounter(gpuPt.queries[_gpuFrame][1], GL_TIMESTAMP);
    }
}

void Profiler::swapFrames()
{
    assert(_initialized);

    for(CPUProfilePoint& pt : _cpuPoints)
    {
        pt.elapsedNs = CpuDuration(pt.stop - pt.start).count();
    }

    for(GPUProfilePoint& pt : _gpuPoints)
    {
        GLuint64 start, stop;
        glGetQueryObjectui64v(pt.queries[_gpuFrame][0], GL_QUERY_RESULT, &start);
        glGetQueryObjectui64v(pt.queries[_gpuFrame][1], GL_QUERY_RESULT, &stop);
        pt.elapsedNs = stop - start;
    }

    _gpuFrame = (++_gpuFrame) % 2;
}

void Profiler::startCpuPoint(ProfileIdCpu id)
{
    assert(_initialized);
    assert(id < _cpuPoints.size());
    _cpuPoints[id].start = CpuTimeNow();
}

void Profiler::stopCpuPoint(ProfileIdCpu id)
{
    assert(_initialized);
    assert(id < _cpuPoints.size());
    _cpuPoints[id].stop = CpuTimeNow();
}

void Profiler::startGpuPoint(ProfileIdCpu id)
{
    assert(_initialized);
    assert(id < _gpuPoints.size());
    glQueryCounter(_gpuPoints[id].queries[_gpuFrame][0], GL_TIMESTAMP);
}

void Profiler::stopGpuPoint(ProfileIdCpu id)
{
    assert(_initialized);
    assert(id < _gpuPoints.size());
    glQueryCounter(_gpuPoints[id].queries[_gpuFrame][1], GL_TIMESTAMP);
}

void Profiler::renderUi()
{
    if(ImGui::TreeNode("CPU"))
    {
        for(const CPUProfilePoint& pt : _cpuPoints)
            ImGui::Text("%s: %.3gms", pt.name.c_str(), pt.elapsedNs / 1e6f);

        ImGui::TreePop();
    }

    if(ImGui::TreeNode("GPU"))
    {
        for(const GPUProfilePoint& pt : _gpuPoints)
            ImGui::Text("%s: %.3gms", pt.name.c_str(), pt.elapsedNs / 1e6f);

        ImGui::TreePop();
    }
}

}
