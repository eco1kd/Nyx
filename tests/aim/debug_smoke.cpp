#include <cassert>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include "features/aim/runtime.hpp"
static std::vector<std::string> lines;
extern "C" int __android_log_print(int,const char* tag,const char* fmt,...){char b[1600];va_list a;va_start(a,fmt);vsnprintf(b,sizeof b,fmt,a);va_end(a);assert(std::strcmp(tag,"LemmingRMT")==0);lines.emplace_back(b);return 0;}
static int normalOriginalCalls=0,silentOriginalCalls=0,inputOriginalCalls=0;
static void inputOriginal(void*,lemming::aim::runtime::ExecuteTime,const void*){++inputOriginalCalls;}
static void cameraOriginal(void*,lemming::aim::runtime::ExecuteTime,const void*){++normalOriginalCalls;}
static void silentOriginal(void*,void*,lemming::aim::runtime::Ray*,std::uint64_t){++silentOriginalCalls;}
int main(){using namespace lemming::aim;namespace rt=runtime;Configuration c;publish(c);rt::onPlayer(nullptr);assert(debug::playerCalls==1&&debug::inactive==1);c.normal.enabled=true;publish(c);rt::onPlayer(nullptr);assert(debug::runtimeNotReady==1);rt::g_cameraOriginal.store(cameraOriginal);rt::g_emitOriginal=silentOriginal;rt::cameraTick(nullptr,{1,.016f},nullptr);rt::emitRays(nullptr,nullptr,nullptr,0);assert(normalOriginalCalls==1&&silentOriginalCalls==1);assert(debug::cameraCalls==1&&debug::silentCalls==1);assert(debug::silentReason==1);debug::report();assert(lines.size()==8);rt::g_commandGateOriginal.store(+[](void*,void*,const void*){++inputOriginalCalls;return true;});assert(rt::commandGate(nullptr,nullptr,nullptr));assert(inputOriginalCalls==1);assert(lines[0].find("build=v16-command-phase-hitbox")!=std::string::npos);assert(lines[0].find("enabled[angle=1 silent=0]")!=std::string::npos);assert(lines[1].find("runtimeNotReady=1")!=std::string::npos);assert(lines[2].find("run[angle=wrong-thread silent=disabled]")!=std::string::npos);for(auto& line:lines)std::puts(line.c_str());std::puts("PASS diagnostic formatting, callback counters, disabled/runtime guards and original-call passthrough");}
