#ifndef PROFILER_H
#define PROFILER_H

#include <string>
#include <vector>

struct ImVec2;
class ImDrawList;

namespace unisim
{

typedef unsigned int ProfileIdCpu;
typedef unsigned int ProfileIdGpu;


struct CPUProfilePoint;
struct GPUProfilePoint;

class Profiler
{
    struct ProfileNode
    {
        int profileId;
        int parent;
        std::vector<unsigned int> children;
    };

    struct ResolvedPoint
    {
        const char* name;
        uint64_t recStartNs;
        uint64_t recStopNs;
        uint64_t recElapsedNs;
        std::vector<unsigned int> children;
    };

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

    void startGpuPoint(ProfileIdGpu id);
    void stopGpuPoint(ProfileIdGpu id);

    void ui();

private:
    void renderChildrenText(const std::vector<ResolvedPoint>& tree, int nodeId);
    float renderChildrenRect(const std::vector<ResolvedPoint>& tree, int nodeId, double scale, double bias, const ImVec2& p, ImDrawList* draw_list);
    void renderPointRect(const ResolvedPoint& pt, double scale, double bias, const ImVec2& p, ImDrawList* draw_list);

    bool _initialized;
    unsigned int _frame;

    std::vector<CPUProfilePoint> _cpuPoints;
    std::vector<GPUProfilePoint> _gpuPoints;

    ResolvedPoint _renderedSyncPt;

    unsigned int _currCpuNode;
    std::vector<ProfileNode> _activeCpuTree;
    std::vector<ProfileNode> _completedCpuTree;
    std::vector<ResolvedPoint> _renderedCpuTree;
    unsigned int _currGpuNode;
    std::vector<ProfileNode> _activeGpuTree;
    std::vector<ProfileNode> _completedGpuTree;
    std::vector<ResolvedPoint> _renderedGpuTree;
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

#define PID_CPU(name) ProfileIdCpu_##name
#define DefineProfilePoint(name) static ProfileIdCpu PID_CPU(name) = Profiler::GetInstance().registerCpuPoint(#name)
#define Profile(name) ScoppedCpuPoint profilePoint_##name(PID_CPU(name))

#define PID_GPU(name) ProfileIdGpu_##name
#define DefineProfilePointGpu(name) static ProfileIdGpu PID_GPU(name) = Profiler::GetInstance().registerGpuPoint(#name)
#define ProfileGpu(name) ScoppedGpuPoint profilePointGpu_##name(PID_GPU(name))

}

#endif // PROFILER_H
