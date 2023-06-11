#include "profiler.h"

#include <chrono>
#include <cassert>

#include <GLM/glm.hpp>

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
    CpuTime start[2];
    CpuTime stop[2];
};

struct GPUProfilePoint
{
    ProfileIdCpu id;
    std::string name;

    GLuint start[2];
    GLuint stop[2];
};

CpuTime g_cpuEpoch = CpuTimeNow();
CPUProfilePoint g_SyncPts;
CPUProfilePoint g_CpuFrame;
GPUProfilePoint g_GpuFrame;


CPUProfilePoint createCpuPoint(ProfileIdCpu id, const std::string& name)
{
    CPUProfilePoint pt;
    pt.id = id;
    pt.name = name;
    return pt;
}

GPUProfilePoint createGpuPoint(ProfileIdGpu id, const std::string& name)
{
    GPUProfilePoint pt;
    pt.id = id;
    pt.name = name;
    pt.start[0] = 0;
    pt.start[1] = 0;
    pt.stop[0] = 0;
    pt.stop[1] = 0;
    return pt;
}

void initializeCpuPoint(CPUProfilePoint& pt)
{
    auto now = CpuTimeNow();
    pt.start[0] = now;
    pt.stop[0]  =  now;
    pt.start[1] = now;
    pt.stop[1]  =  now;
}

void initializeGpuPoint(GPUProfilePoint& pt)
{
    glGenQueries(2, pt.start);
    glGenQueries(2, pt.stop);
    glQueryCounter(pt.start[0], GL_TIMESTAMP);
    glQueryCounter(pt.stop[0], GL_TIMESTAMP);
    glQueryCounter(pt.start[1], GL_TIMESTAMP);
    glQueryCounter(pt.stop[1], GL_TIMESTAMP);
}

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
    _cpuPoints.push_back(createCpuPoint(id, name));

    return id;
}

ProfileIdGpu Profiler::registerGpuPoint(const std::string& name)
{
    assert(!_initialized);

    ProfileIdGpu id = ProfileIdGpu(_gpuPoints.size());
    _gpuPoints.push_back(createGpuPoint(id, name));

    return id;
}

void Profiler::initize()
{
    _initialized = true;

    _frame = 0;

    initializeCpuPoint(g_CpuFrame);

    initializeGpuPoint(g_GpuFrame);

    for(CPUProfilePoint& pt: _cpuPoints)
    {
        initializeCpuPoint(pt);
    }

    for(GPUProfilePoint& pt : _gpuPoints)
    {
        initializeGpuPoint(pt);
    }

    _currCpuNode = 0;
    _activeCpuTree.clear();
    _activeCpuTree.push_back({-1, -1, {}});
    _completedCpuTree = _activeCpuTree;

    _currGpuNode = 0;
    _activeGpuTree.clear();
    _activeGpuTree.push_back({-1, -1, {}});
    _completedGpuTree = _activeGpuTree;
}

typedef std::vector<unsigned char> BitField;

bool IsSet(const BitField& bitField, int id)
{
    return bitField[id >> 3] & (1 << (id & 0xff));
}

void Set(BitField& bitField, int id)
{
    bitField[id >> 3] |= 1 << (id & 0xff);
}

void Profiler::swapFrames()
{
    assert(_initialized);

    // Close previous frame
    g_CpuFrame.stop[_frame] = CpuTimeNow();
    glQueryCounter(g_GpuFrame.stop[_frame], GL_TIMESTAMP);

    // Switch frame
    _frame = (++_frame) % 2;

    auto resolveCpuPt = [&](CPUProfilePoint& pt, unsigned int frame, const ProfileNode& node) -> ResolvedPoint
    {
        ResolvedPoint resolve;
        resolve.name = node.profileId != -1 ? _cpuPoints[node.profileId].name.c_str() : "Frame";
        resolve.recStartNs = (pt.start[frame] - g_cpuEpoch).count();
        resolve.recStopNs = (pt.stop[frame] - g_cpuEpoch).count();
        resolve.recElapsedNs = (pt.stop[frame] - pt.start[frame]).count();
        resolve.children = node.children;
        return resolve;
    };

    auto resolveGpuPt = [&] (GPUProfilePoint& pt, unsigned int frame, const ProfileNode& node) -> ResolvedPoint
    {
        ResolvedPoint resolve;
        resolve.name = node.profileId != -1 ? _gpuPoints[node.profileId].name.c_str() : "Frame";
        glGetQueryObjectui64v(pt.start[frame], GL_QUERY_RESULT, &resolve.recStartNs);
        glGetQueryObjectui64v(pt.stop[frame], GL_QUERY_RESULT, &resolve.recStopNs);
        resolve.recElapsedNs = resolve.recStopNs - resolve.recStartNs;
        resolve.children = node.children;
        return resolve;
    };

    // Resolve second to last sync point
    ProfileNode dummySyncNode = {-1, -1, {}};
    _renderedSyncPt = resolveCpuPt(g_SyncPts, _frame, dummySyncNode);
    _renderedSyncPt.name = "Sync Points";

    // Start this sync pt
    g_SyncPts.start[_frame] = CpuTimeNow();

    _renderedCpuTree.clear();
    _renderedGpuTree.clear();

    // CPU Frame
    {
        ResolvedPoint resolve = resolveCpuPt(g_CpuFrame, _frame, _completedCpuTree[0]);
        _renderedCpuTree.push_back(resolve);
    }

    // GPU Frame
    {
        ResolvedPoint resolve = resolveGpuPt(g_GpuFrame, _frame, _completedGpuTree[0]);
        _renderedGpuTree.push_back(resolve);
    }

    BitField cpuBitField((_cpuPoints.size() +7) / 8, 0);
    for(const ProfileNode& node : _completedCpuTree)
    {
        if(node.profileId != -1)
        {
            // Cannot use same point multiple times in a frame
            assert(!IsSet(cpuBitField, node.profileId));
            Set(cpuBitField, node.profileId);

            ResolvedPoint resolve = resolveCpuPt(_cpuPoints[node.profileId], _frame, node);
            _renderedCpuTree.push_back( resolve );
        }
    }

    BitField gpuBitField((_gpuPoints.size() +7) / 8, 0);
    for(const ProfileNode& node : _completedGpuTree)
    {
        if(node.profileId != -1)
        {
            // Cannot use same point multiple times in a frame
            assert(!IsSet(gpuBitField, node.profileId));
            Set(gpuBitField, node.profileId);

            ResolvedPoint resolvedPt = resolveGpuPt(_gpuPoints[node.profileId], _frame, node);
            _renderedGpuTree.push_back(resolvedPt);
        }
    }

    // Move active tree to buffer completed tree
    // Completed tree will be resolve the next frame
    _completedCpuTree = _activeCpuTree;
    _completedGpuTree = _activeGpuTree;

    _currCpuNode = 0;
    _activeCpuTree.clear();
    _activeCpuTree.push_back({-1, -1, {}});

    _currGpuNode = 0;
    _activeGpuTree.clear();
    _activeGpuTree.push_back({-1, -1, {}});

    g_SyncPts.stop[_frame] = CpuTimeNow();

    g_CpuFrame.start[_frame] = CpuTimeNow();
    glQueryCounter(g_GpuFrame.start[_frame], GL_TIMESTAMP);

}

void Profiler::startCpuPoint(ProfileIdCpu id)
{
    assert(_initialized);
    assert(id < _cpuPoints.size());
    _cpuPoints[id].start[_frame] = CpuTimeNow();

    int newNode = _activeCpuTree.size();
    _activeCpuTree[_currCpuNode].children.push_back(newNode);
    _activeCpuTree.push_back({int(id), int(_currCpuNode), {}});
    _currCpuNode = newNode;
}

void Profiler::stopCpuPoint(ProfileIdCpu id)
{
    assert(_initialized);
    assert(id < _cpuPoints.size());
    _cpuPoints[id].stop[_frame] = CpuTimeNow();

    assert(_activeCpuTree[_currCpuNode].profileId == int(id));
    _currCpuNode = _activeCpuTree[_currCpuNode].parent;
}

void Profiler::startGpuPoint(ProfileIdGpu id)
{
    assert(_initialized);
    assert(id < _gpuPoints.size());
    glQueryCounter(_gpuPoints[id].start[_frame], GL_TIMESTAMP);

    int newNode = _activeGpuTree.size();
    _activeGpuTree[_currGpuNode].children.push_back(newNode);
    _activeGpuTree.push_back({int(id), int(_currGpuNode), {}});
    _currGpuNode = newNode;
}

void Profiler::stopGpuPoint(ProfileIdGpu id)
{
    assert(_initialized);
    assert(id < _gpuPoints.size());
    glQueryCounter(_gpuPoints[id].stop[_frame], GL_TIMESTAMP);

    assert(_activeGpuTree[_currGpuNode].profileId == int(id));
    _currGpuNode = _activeGpuTree[_currGpuNode].parent;
}

const float BOX_HEIGHT = 20;
const float BOX_SPACING = 4;

void Profiler::renderUi()
{
    const ResolvedPoint& cpuFrameNode = _renderedCpuTree[0];
    const ResolvedPoint& gpuFrameNode = _renderedGpuTree[0];

    double windowWidth = ImGui::GetWindowSize().x;

    double frameScale = windowWidth / glm::max(cpuFrameNode.recElapsedNs + _renderedSyncPt.recElapsedNs, gpuFrameNode.recElapsedNs);
    double cpuFrameBias = - frameScale * (cpuFrameNode.recStartNs - _renderedSyncPt.recElapsedNs);
    double gpuFrameBias = - frameScale * (gpuFrameNode.recStartNs - _renderedSyncPt.recElapsedNs);

    if(ImGui::TreeNode("Graph"))
    {
        ImDrawList* draw_list = ImGui::GetWindowDrawList();

        ImVec2 originalPosition = ImGui::GetCursorScreenPos();
        ImVec2 p = originalPosition;

        renderPointRect(_renderedSyncPt, frameScale, cpuFrameBias, p, draw_list);
        renderPointRect(cpuFrameNode, frameScale, cpuFrameBias, p, draw_list);
        p.y += BOX_HEIGHT + BOX_SPACING;
        p.y += renderChildrenRect(_renderedCpuTree, 0, frameScale, cpuFrameBias, p, draw_list);
        p.y += BOX_HEIGHT;

        renderPointRect(gpuFrameNode, frameScale, gpuFrameBias, p, draw_list);
        p.y += BOX_HEIGHT + BOX_SPACING;
        p.y += renderChildrenRect(_renderedGpuTree, 0, frameScale, gpuFrameBias, p, draw_list);
        p.y += BOX_SPACING;

        ImGui::Dummy(ImVec2(windowWidth, (p.y - originalPosition.y)));

        ImGui::TreePop();
    }

    if(ImGui::TreeNode("CPU"))
    {
        ImGui::Text("   Sync Timelines %.3gms", _renderedSyncPt.recElapsedNs / 1e6f);
        if(ImGui::TreeNode("CPUFrame", "Frame %.3gms", cpuFrameNode.recElapsedNs / 1e6f))
        {
            renderChildrenText(_renderedCpuTree, 0);
            ImGui::TreePop();
        }

        ImGui::TreePop();
    }

    if(ImGui::TreeNode("GPU"))
    {
        if(ImGui::TreeNode("GPUFrame", "Frame %.3gms", gpuFrameNode.recElapsedNs / 1e6f))
        {
            renderChildrenText(_renderedGpuTree, 0);
            ImGui::TreePop();
        }

        ImGui::TreePop();
    }
}

void Profiler::renderChildrenText(const std::vector<ResolvedPoint>& tree, int nodeId)
{
    const auto& children = tree[nodeId].children;

    for(int c : children)
    {
        const ResolvedPoint& pt = tree[c];

        if(pt.children.empty())
        {
            ImGui::Text("   %s %.3gms", pt.name, pt.recElapsedNs / 1e6f);
            continue;
        }

        if(ImGui::TreeNode(pt.name, "%s %.3gms", pt.name, pt.recElapsedNs / 1e6f))
        {
            renderChildrenText(tree, c);
            ImGui::TreePop();
        }
    }
}

float Profiler::renderChildrenRect(const std::vector<ResolvedPoint>& tree, int nodeId, double scale, double bias, const ImVec2& p, ImDrawList* draw_list)
{
    float maxDepth = 0.0f;

    const auto& children = tree[nodeId].children;
    ImVec2 childrenP = p;
    childrenP.y += BOX_HEIGHT + BOX_SPACING;

    for(int c : children)
    {
        const ResolvedPoint& pt = tree[c];
        renderPointRect(pt, scale, bias, p, draw_list);
        float childDepth = renderChildrenRect(tree, c, scale, bias, childrenP, draw_list);
        maxDepth = glm::max(maxDepth, childDepth + BOX_HEIGHT + BOX_SPACING);
    }

    return maxDepth;
}

unsigned char charToChannel(unsigned char c)
{
    return (c % 32) * (256 / 32);
}

void Profiler::renderPointRect(const ResolvedPoint& pt, double scale, double bias, const ImVec2 &p, ImDrawList* draw_list)
{
    ImVec2 min = p;
    min.x += pt.recStartNs * scale + bias;

    ImVec2 max = p;
    max.x += pt.recStopNs * scale + bias;
    max.y += BOX_HEIGHT;

    ImColor color(charToChannel(pt.name[0]), charToChannel(pt.name[1]), charToChannel(pt.name[2]));

    draw_list->AddRectFilled(min, max, color);
}

}
