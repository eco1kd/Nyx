#include <cassert>
#include <cstdarg>
#include <cstring>
#include <cstdio>
#include <limits>
#include "features/aim/runtime.hpp"
extern "C" int __android_log_print(int,const char*,const char*,...){return 0;}
namespace rt=lemming::aim::runtime;namespace ed=lemming::esp::detail;
using namespace lemming::aim;
struct alignas(8) Buffer {unsigned char b[256]{};};
static Buffer commandClass,scopeClass,gunClass,weaponClass,knifeClass;
static const char* className(void* c){return c==&commandClass?"owv":c==&scopeClass?"GunWithScopeParameters":c==&gunClass?"GunParameters":c==&weaponClass?"qjf":"KnifeParameters";}
template<class T>void put(Buffer& b,int offset,T value){std::memcpy(b.b+offset,&value,sizeof(value));}
static void attack(void* p,bool value,const void*){static_cast<unsigned char*>(p)[0x32]=value;}
static void scope(void* p,bool value,const void*){static_cast<unsigned char*>(p)[0x33]=value;}
static void hold(void* p,bool value,const void*){static_cast<unsigned char*>(p)[0x34]=value;}
static int originals=0;static bool original(void*,void*,const void*){++originals;return true;}
static int hitId=42;static bool hits=true;static float hitDistance=10;
static std::uint64_t scene(const void*){return 0;}
static bool cast(const std::uint64_t*,const rt::Ray*,assists::RaycastHit* out,float,int mask,int query){assert(mask==-5&&query==2);out->collider=hitId;out->distance=hitDistance;return hits;}
int main(){
 static_assert(sizeof(rt::CommandLook)==12&&offsetof(rt::CommandLook,pitch)==4&&offsetof(rt::CommandLook,yaw)==8,"nullable command ABI");
 prctl(PR_SET_NAME,"UnityMain",0,0,0);assert(ed::isUnityMainCallback());ed::refreshMaps(true);ed::g_runtime.api.classGetName=className;
 Buffer cmd,definition,weapon,local,remote,input;put(cmd,0,&commandClass);put(definition,0,&scopeClass);put(weapon,0,&weaponClass);put(weapon,0x60,&definition);assert(rt::autoGunKind(&weapon)==2);put(definition,0,&gunClass);assert(rt::autoGunKind(&weapon)==1);put(definition,0,&knifeClass);assert(rt::autoGunKind(&weapon)==0);
 rt::g_commandAttack=attack;rt::g_commandScope=scope;rt::g_commandScopeHold=hold;rt::g_ownedCommand=&cmd;rt::g_ownedAttack=rt::g_ownedScope=rt::g_ownedScopeHold=true;rt::g_previousAttack=1;rt::g_previousScope=rt::g_previousScopeHold=0;cmd.b[0x32]=cmd.b[0x33]=cmd.b[0x34]=1;rt::releaseOwnedCommand();assert(cmd.b[0x32]==1&&cmd.b[0x33]==0&&cmd.b[0x34]==0&&!rt::g_ownedCommand&&!rt::g_ownedAttack);
 rt::g_commandGateOriginal.store(original);{std::lock_guard<std::mutex> lock(ed::g_hookPlayerMutex);ed::g_hookLocalPlayer=&local;}put(input,0x18,&remote);put(input,0x40,&cmd);rt::g_ownedCommand=&cmd;rt::g_ownedAttack=true;assert(rt::commandGate(&input,&cmd,nullptr));assert(originals==1&&rt::g_ownedAttack&&cmd.b[0x32]==1);rt::g_ownedCommand=nullptr;rt::g_ownedAttack=false;
 g_visibilityReady.store(true);rt::g_raycast=cast;rt::g_defaultScene=scene;Selection target{};target.valid=true;target.player=reinterpret_cast<std::uintptr_t>(&remote);auto& actor=rt::g_actors[0];actor.player=&remote;actor.world=&local;actor.team=2;actor.hp=100;actor.captured=rt::Clock::now();actor.colliderIds[0]=42;actor.colliderCount=1;
 assert(rt::autoAligned(target,&local,1,{0,0,0},{0,0,1},150));hitId=99;assert(!rt::autoAligned(target,&local,1,{0,0,0},{0,0,1},150));hitId=42;hits=false;assert(!rt::autoAligned(target,&local,1,{0,0,0},{0,0,1},150));hits=true;hitDistance=std::numeric_limits<float>::quiet_NaN();assert(!rt::autoAligned(target,&local,1,{0,0,0},{0,0,1},150));hitDistance=10;actor.captured=rt::Clock::now()-std::chrono::seconds(1);assert(!rt::autoAligned(target,&local,1,{0,0,0},{0,0,1},150));actor.captured=rt::Clock::now();assert(!rt::autoAligned(target,&local,2,{0,0,0},{0,0,1},150));
 prctl(PR_SET_NAME,"host-tests",0,0,0);assert(rt::commandGate(&input,&cmd,nullptr));assert(originals==2);assert(!rt::autoAligned(target,&local,1,{0,0,0},{0,0,1},150));
 puts("PASS command ABI, scoped/unscoped/knife filtering, owned button release preserving physical holds, remote/wrong-thread original passthrough, real first-hit/blocked/clear/stale/team/nonfinite ray gates");
}
