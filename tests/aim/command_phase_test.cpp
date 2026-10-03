#include "features/aim/runtime.hpp"
#include <cassert>
#include <cstring>
#include <cstdio>
extern "C" int __android_log_print(int,const char*,const char*,...){return 0;}
namespace rt=lemming::aim::runtime;namespace ed=lemming::esp::detail;using namespace lemming::aim;
struct alignas(8) Buffer {unsigned char bytes[512]{};};
static Buffer inputClass,commandClass;static int gateCalls=0,endCalls=0;static bool expectedScope=false;
static const char* name(void* p){return p==&inputClass?"ltc":p==&commandClass?"owv":"other";}
template<class T>void put(Buffer& b,int o,T v){std::memcpy(b.bytes+o,&v,sizeof(v));}
static bool gate(void*,void*,const void*){++gateCalls;return false;}
static void end(void* self,const void*){++endCalls;if(self==rt::g_phaseLease.self&&expectedScope){std::uint8_t s=0;rt::field(rt::g_phaseLease.cmd,0x33,s);assert(s==1);}}
int main(){
 prctl(PR_SET_NAME,"UnityMain",0,0,0);ed::refreshMaps(true);ed::g_runtime.api.classGetName=name;
 Buffer local,other,world,self,remote,cmd;put(local,0x18,&world);put(self,0,&inputClass);put(self,0x18,&local);put(self,0x20,&world);put(self,0x28,std::uint8_t(1));put(cmd,0,&commandClass);put(cmd,0x10,std::uint32_t(7));
 {std::lock_guard<std::mutex> l(ed::g_hookPlayerMutex);ed::g_hookLocalPlayer=&local;}
 rt::g_commandGateOriginal.store(gate);rt::g_commandEndOriginal.store(end);rt::g_commandAttack=+[](void* c,bool v,const void*){std::memcpy(static_cast<char*>(c)+0x32,&v,1);};rt::g_commandScope=+[](void* c,bool v,const void*){std::memcpy(static_cast<char*>(c)+0x33,&v,1);};rt::g_commandScopeHold=+[](void* c,bool v,const void*){std::memcpy(static_cast<char*>(c)+0x34,&v,1);};
 g_inputHookReady.store(false);assert(!rt::commandGate(&self,&cmd,nullptr)&&gateCalls==1);
 rt::CommandLook before{1,{0,0,0},3,42};put(cmd,0x1c,before);assert(rt::beginCommandPhase(&self,&cmd));rt::g_phaseLease.lookOwned=true;put(cmd,0x1c,rt::CommandLook{1,{0,0,0},-20,150});rt::g_ownedCommand=&cmd;rt::g_ownedScope=rt::g_ownedScopeHold=true;rt::g_previousScope=false;rt::g_previousScopeHold=true;put(cmd,0x33,std::uint8_t(1));put(cmd,0x34,std::uint8_t(1));expectedScope=true;
 rt::commandEnd(&remote,nullptr);assert(endCalls==1&&rt::g_phaseLease.cmd==&cmd);rt::commandEnd(&self,nullptr);assert(endCalls==2&&!rt::g_phaseLease.cmd);rt::CommandLook after{};assert(rt::field(&cmd,0x1c,after)&&!std::memcmp(&before,&after,12));assert(cmd.bytes[0x33]==0&&cmd.bytes[0x34]==1);
 expectedScope=false;assert(rt::beginCommandPhase(&self,&cmd));rt::g_phaseLease.lookOwned=true;rt::g_ownedCommand=&cmd;rt::g_ownedAttack=true;rt::g_previousAttack=false;put(cmd,0x10,std::uint32_t(8));put(cmd,0x32,std::uint8_t(1));auto copy=cmd;rt::commandEnd(&self,nullptr);assert(!std::memcmp(copy.bytes,cmd.bytes,512)&&!rt::g_phaseLease.cmd);
 put(self,0x18,&other);assert(!rt::beginCommandPhase(&self,&cmd));put(self,0x18,&local);put(self,0x28,std::uint8_t(0));assert(!rt::beginCommandPhase(&self,&cmd));put(self,0x28,std::uint8_t(1));put(self,0x20,&other);assert(!rt::beginCommandPhase(&self,&cmd));put(self,0x20,&world);
 prctl(PR_SET_NAME,"host-test",0,0,0);assert(!rt::beginCommandPhase(&self,&cmd));assert(!rt::commandGate(&self,&cmd,nullptr)&&gateCalls==2);
 puts("PASS unchanged original gate return, original end exactly once before restore, local/world/thread ownership, scope survives late consumer, 12-byte Look restore, physical hold preservation, producer-token reuse no-write");
}
