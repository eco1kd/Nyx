#include <cassert>
#include <cstring>
#include <limits>
#include <cstdio>
#include "features/aim/runtime.hpp"
extern "C" int __android_log_print(int,const char*,const char*,...){return 0;}
namespace rt=lemming::aim::runtime;namespace ed=lemming::esp::detail;
using namespace lemming::aim;
struct alignas(8) Buffer {unsigned char data[512]{};};
static Buffer containerClass,inputClass,cameraClass;
static const char* getName(void* c){return c==&containerClass?"wmz":c==&inputClass?"uys":c==&cameraClass?"ios":"other";}
template<class T>void put(Buffer& b,int offset,T v){std::memcpy(b.data+offset,&v,sizeof(v));}
int main(){
 input_init::Retry retry;assert(!retry.due(100,true,false,0));assert(retry.due(100,true,false,1));for(int i=101;i<1100;++i)assert(!retry.due(i,true,false,1));assert(retry.due(1100,true,false,1));assert(retry.due(1101,true,false,2));assert(!retry.due(1200,true,true,2));assert(!retry.due(1200,false,false,2));assert(retry.due(1201,true,false,2));assert(!retry.due(std::numeric_limits<double>::quiet_NaN(),true,false,2));assert(retry.due(10,true,false,2));
 prctl(PR_SET_NAME,"UnityMain",0,0,0);ed::refreshMaps(true);ed::g_runtime.api.classGetName=getName;
 Buffer local,remote,world,wrongWorld,container,arr,input,camera,dict,entries;
 put(local,0x18,&world);put(local,0x20,&container);put(container,0,&containerClass);put(input,0,&inputClass);put(input,0x18,&local);put(input,0x20,&world);put(camera,0,&cameraClass);put(camera,0x18,&local);put(camera,0x20,&world);
 put(arr,0x18,std::uintptr_t(2));put(arr,0x20,&input);put(arr,0x28,&camera);
 for(int offset:{0x10,0x18,0x20,0x28,0x30,0x38}){put(container,offset,&arr);assert(rt::component(&local,"uys")==&input);assert(rt::component(&local,"ios")==&camera);put(container,offset,static_cast<void*>(nullptr));}
 put(container,0x40,&dict);put(dict,0x18,&entries);put(dict,0x20,std::int32_t(2));put(entries,0x18,std::uintptr_t(2));put(entries,0x20,std::int32_t(-1));put(entries,0x30,&input);put(entries,0x38,std::int32_t(17));put(entries,0x48,&input);
 input_init::ComponentStats stats;assert(rt::component(&local,"uys",&stats)==&input&&stats.dictionary&&stats.entries==1);put(input,0x18,&remote);assert(!rt::component(&local,"uys",&stats)&&stats.foreign==1);put(input,0x18,&local);put(input,0x20,&wrongWorld);assert(!rt::component(&local,"uys"));put(input,0x20,&world);
 put(entries,0x18,std::uintptr_t(1));assert(!rt::component(&local,"uys"));put(entries,0x18,std::uintptr_t(2));put(dict,0x20,std::int32_t(999));assert(!rt::component(&local,"uys"));put(dict,0x20,std::int32_t(2));put(container,0,static_cast<void*>(nullptr));assert(!rt::component(&local,"uys"));put(container,0,&containerClass);
 {std::lock_guard<std::mutex> lock(ed::g_hookPlayerMutex);ed::g_hookLocalPlayer=&local;}
 ed::g_runtime.ready=true;rt::g_functionsValid=true;g_active.store(true);{std::lock_guard<std::mutex> lock(g_configMutex);g_config.autoFire.enabled=true;g_config.normal.enabled=g_config.silent.enabled=false;}
 g_inputHookReady.store(false);rt::g_inputRetry={};rt::g_lastInstall=rt::Clock::now();rt::g_lastAssistBind=rt::Clock::now();auto previous=debug::inputAttempts.load();rt::onPlayer(&remote);assert(debug::inputAttempts.load()==previous+1&&g_inputInstallReason.load()==input_init::UnsupportedArchitecture);rt::onPlayer(&remote);rt::onPlayer(&local);assert(debug::inputAttempts.load()==previous+1);
 g_inputHookReady.store(true);rt::g_inputRetry={};rt::onPlayer(&remote);assert(debug::inputAttempts.load()==previous+1);g_inputHookReady.store(false);
 {std::lock_guard<std::mutex> lock(ed::g_hookPlayerMutex);ed::g_hookLocalPlayer=nullptr;}rt::onPlayer(&remote);assert(g_inputInstallReason.load()==input_init::NoLocal);{std::lock_guard<std::mutex> lock(ed::g_hookPlayerMutex);ed::g_hookLocalPlayer=&local;}
 prctl(PR_SET_NAME,"host-test",0,0,0);assert(!rt::installInput(&local)&&g_inputInstallReason.load()==input_init::WrongThread);
 puts("PASS independent local-input retry, remote-callback initialization despite busy common timer, six component arrays/dictionary fallback, deleted/foreign/world/invalid bounds guards, and explicit install failures");
}
