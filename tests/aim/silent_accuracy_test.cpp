#include "features/aim/runtime.hpp"
#include <cassert>
#include <cstring>
#include <cstdio>
#include <limits>
extern "C" int __android_log_print(int,const char*,const char*,...){return 0;}
namespace rt=lemming::aim::runtime;namespace ed=lemming::esp::detail;using namespace lemming::aim;
struct alignas(8) Buffer {unsigned char bytes[512]{};};
static Buffer playerClass,jgiClass,hbClass,colliderClass;static rt::Bounds liveBounds{{0,1.6f,10},{.1f,.16f,.12f}};static int hitId=42,boundReads=0;
static const char* name(void* c){return c==&playerClass?"ake":c==&jgiClass?"jgi":c==&hbClass?"HitBoxCollider":c==&colliderClass?"BoxCollider":"other";}
template<class T>void put(Buffer& b,int o,T v){std::memcpy(b.bytes+o,&v,sizeof(v));}
static void bounds(std::uintptr_t,rt::Bounds* b){++boundReads;*b=liveBounds;}
static bool cast(const std::uint64_t*,const rt::Ray*,assists::RaycastHit* h,float,int,int){h->collider=hitId;h->distance=1;return hitId!=0;}
int main(){
 HitVolume head{{0,0,10},{.1f,.16f,.12f},10};for(int i=0;i<1000;++i){auto p=silent_accuracy::innerPoint(head,{float(i),float(-i),float(i+10)});assert(finite(p)&&length(sub(p,head.center))<=.0351f);}auto p=silent_accuracy::innerPoint(head,{std::numeric_limits<float>::quiet_NaN(),0,0});assert(p.z==10&&p.x==0);
 Settings s;s.fov=180;s.maxDistance=150;assert(silent_accuracy::inFov({0,0,0},{0,0,-10},{0,0,1},s));s.fov=8;assert(!silent_accuracy::inFov({0,0,0},{10,0,0},{0,0,1},s));assert(!silent_accuracy::inFov({0,0,0},{0,0,1000},{0,0,1},s));
 prctl(PR_SET_NAME,"UnityMain",0,0,0);ed::refreshMaps(true);ed::g_runtime.api.classGetName=name;ed::g_runtime.playerClass=&playerClass;ed::g_runtime.unityBase=0x10000000;
 Buffer player,world,parts,list,arr,jgi,hitboxes,hb,collider,native,vt;put(player,0,&playerClass);put(player,0x18,&world);put(player,0x28,&parts);put(parts,0x10,&list);put(list,0x10,&arr);put(list,0x18,std::int32_t(1));put(arr,0x18,std::uintptr_t(1));put(arr,0x20,&jgi);put(jgi,0,&jgiClass);put(jgi,0x18,&hitboxes);put(hitboxes,0x18,std::uintptr_t(1));put(hitboxes,0x20,&hb);put(hb,0,&hbClass);put(hb,0x20,std::int32_t(10));put(hb,0x28,&collider);put(collider,0,&colliderClass);put(collider,0x10,&native);put(native,0,&vt);auto code=std::uintptr_t(0x10000000+0x535F940+4096);put(vt,0,code);put(vt,0xf0,code);
 auto now=rt::Clock::now();{std::lock_guard<std::mutex> l(ed::g_playerPoseMutex);ed::g_playerPoses[0]={&player,&world,{0,0,10},now};}
 rt::g_bounds=bounds;rt::g_entityId=+[](void*,const void*){return 42;};rt::g_defaultScene=+[](const void*)->std::uint64_t{return 0;};rt::g_raycast=cast;g_visibilityReady.store(true);
 Selection selected{};selected.valid=true;selected.player=reinterpret_cast<std::uintptr_t>(&player);selected.bone=10;selected.point={-2,1.6f,10};selected.volume={{-2,1.6f,10},{.1f,.16f,.12f},10};s.fov=30;s.prediction=false;s.multipoints=true;Vec3 point{};std::int32_t colliderId=0;
 assert(rt::silentLivePoint(selected,s,{0,0,0},{0,0,1},point,colliderId)&&boundReads==1&&point.x==0&&point.y==1.6f&&colliderId==42);
 s.visibleCheck=true;hitId=99;assert(!rt::silentLivePoint(selected,s,{0,0,0},{0,0,1},point,colliderId));hitId=0;assert(!rt::silentLivePoint(selected,s,{0,0,0},{0,0,1},point,colliderId));hitId=42;
 auto& actor=rt::g_actors[0];actor={};actor.player=&player;actor.world=&world;actor.motion.samples=2;actor.motion.sampled=std::chrono::duration<double>(now.time_since_epoch()).count();actor.motion.velocity={20,0,0};s.prediction=true;s.predictionMs=100;assert(rt::silentLivePoint(selected,s,{0,0,0},{0,0,1},point,colliderId)&&length(sub(point,liveBounds.center))<=.0351f);
 put(hb,0x20,std::int32_t(8));assert(!rt::silentLivePoint(selected,s,{0,0,0},{0,0,1},point,colliderId));put(hb,0x20,std::int32_t(10));put(collider,0x10,static_cast<void*>(nullptr));assert(!rt::silentLivePoint(selected,s,{0,0,0},{0,0,1},point,colliderId));put(collider,0x10,&native);
 {std::lock_guard<std::mutex> l(ed::g_playerPoseMutex);ed::g_playerPoses[0].captured=now-std::chrono::seconds(1);}assert(!rt::silentLivePoint(selected,s,{0,0,0},{0,0,1},point,colliderId));
 puts("PASS refreshed head center, large/NaN lead bounds, 360/FOV/range checks, exact selected-collider visibility (not chest/clear ray), bone/native/stale rejection; no old snapshot age added twice");
}
