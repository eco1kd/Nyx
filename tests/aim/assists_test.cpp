#include <cassert>
#include <cmath>
#include <limits>
#include <cstdio>
#include "features/aim/runtime.hpp"
extern "C" int __android_log_print(int,const char*,const char*,...){return 0;}
using namespace lemming::aim;namespace rt=runtime;namespace a=assists;
static bool close(float x,float y){return std::fabs(x-y)<.002f;}
static bool edge(void*,std::uintptr_t,Vec3,Vec3 p){return p.x>.02f;}
static bool openFar(void*,std::uintptr_t player,Vec3,Vec3){return player==2;}
static int calls=0,hitId=42;static bool collision=true;static float hitDistance=2;
static std::uint64_t scene(const void*){return 0;}
static bool raycast(const std::uint64_t* s,const rt::Ray* r,a::RaycastHit* h,float distance,int mask,int triggers){assert(*s==0&&mask==-5&&triggers==2&&distance>2);assert(close(length(r->direction),1));++calls;h->collider=hitId;h->distance=hitDistance;return collision;}
int main(){
 auto g=a::fovGeometry(8,1920,1080,.9742786f,1.7320508f);assert(g.valid&&!g.coversScreen&&close(g.radiusX,g.radiusY));auto small=a::fovGeometry(8,960,540,.9742786f,1.7320508f);assert(close(small.radiusX*2,g.radiusX));auto zoom=a::fovGeometry(8,1920,1080,1.9485572f,3.4641016f);assert(close(zoom.radiusX,g.radiusX*2));assert(a::fovGeometry(120,1920,1080,1,1).coversScreen);assert(!a::fovGeometry(89,1920,1080,.011f,.011f).coversScreen);assert(!a::fovGeometry(0,1920,1080,1,1).valid);assert(!a::fovGeometry(NAN,1920,1080,1,1).valid);assert(!a::fovGeometry(8,1920,1080,0,1).valid);assert(!a::fovGeometry(8,INFINITY,1080,1,1).valid);
 auto shifted=a::fovGeometry(8,1920,1080,1,1,.1f,.2f);assert(close(shifted.centerX,864)&&close(shifted.centerY,648));
 Angles recoil{};assert(a::recoilAngles({2,1},.5f,recoil)&&close(recoil.pitch,-1)&&close(recoil.yaw,.5));auto aim=a::compensate({0,0},recoil,100);assert(close(aim.pitch,1)&&close(aim.yaw,-.5));assert(close(a::compensate({0,0},recoil,50).pitch,.5));assert(close(a::compensate({0,0},recoil,0).pitch,0));assert(!a::recoilAngles({NAN,1},1,recoil));assert(!a::recoilAngles({50,1},1,recoil));assert(!a::recoilAngles({2,1},-1,recoil));assert(close(a::compensate({88,179},{-5,-10},100).pitch,89));
 auto fully=a::smoothCompensated({0,0},{0,0},{0,0},{-2,1},100,1,.016f);assert(close(fully.pitch,2)&&close(fully.yaw,-1));auto stable=a::smoothCompensated(fully,{0,0},{-2,1},{-2,1},100,1,.016f);assert(close(stable.pitch,2)&&close(stable.yaw,-1));
 a::RecoilTracker tracker;auto d=tracker.delta(1,{-2,1},true);assert(close(d.pitch,-2)&&close(d.yaw,1));d=tracker.delta(1,{-2,1},true);assert(close(d.pitch,0)&&close(d.yaw,0));d=tracker.delta(1,{-3,2},true);assert(close(d.pitch,-1)&&close(d.yaw,1));tracker.delta(1,{-3,2},false);assert(!tracker.active);d=tracker.delta(2,{-1,0},true);assert(close(d.pitch,-1));tracker.synchronize(2,{-2,1});d=tracker.delta(2,{-2,1},true);assert(close(d.pitch,0));d=tracker.delta(2,{-1,.5f},true);assert(close(d.pitch,1)&&close(d.yaw,-.5f));
 Settings cfg;cfg.visibleCheck=true;cfg.multipoints=true;cfg.fov=30;HitVolume volume{{0,0,5},{.2f,.2f,.2f},10};Selection pick{};consider(pick,cfg,1,100,{},{0,0,1},volume);assert(!pick.valid);consider(pick,cfg,1,100,{},{0,0,1},volume,0,edge);assert(pick.valid&&pick.point.x>.02f);pick={};consider(pick,cfg,1,100,{},{0,0,1},volume,0,openFar);assert(!pick.valid);volume.center={.5f,0,8};consider(pick,cfg,2,100,{},{0,0,1},volume,0,openFar);assert(pick.valid&&pick.player==2);cfg.rcsStrength=NAN;assert(sanitize(cfg).rcsStrength==100);cfg.rcsStrength=1000;assert(sanitize(cfg).rcsStrength==100);
 prctl(PR_SET_NAME,"UnityMain",0,0,0);rt::Actor actor;actor.player=reinterpret_cast<void*>(0x1000);actor.colliderIds[0]=42;actor.colliderCount=1;rt::g_raycast=raycast;rt::g_defaultScene=scene;g_visibilityReady.store(true);rt::g_visibilityBudget=64;
 actor.colliderCount=0;assert(!rt::visibilityAccept(&actor,0x1000,{},{0,0,5}));actor.colliderCount=1;assert(rt::visibilityAccept(&actor,0x1000,{},{0,0,5}));hitId=99;assert(!rt::visibilityAccept(&actor,0x1000,{},{0,0,5}));collision=false;assert(rt::visibilityAccept(&actor,0x1000,{},{0,0,5}));collision=true;hitId=42;hitDistance=NAN;assert(!rt::visibilityAccept(&actor,0x1000,{},{0,0,5}));hitDistance=2;g_visibilityReady.store(false);assert(!rt::visibilityAccept(&actor,0x1000,{},{0,0,5}));g_visibilityReady.store(true);rt::g_visibilityBudget=2;int before=calls;for(int i=0;i<10;++i)rt::visibilityAccept(&actor,0x1000,{},{0,0,5});assert(calls-before==2);prctl(PR_SET_NAME,"assists_test",0,0,0);rt::g_visibilityBudget=64;assert(!rt::visibilityAccept(&actor,0x1000,{},{0,0,5}));
 puts("PASS projection FOV/aspect/zoom/invalid inputs, recoil axes/strength/delta/reset, per-point visibility fallback, native ABI/first owned collider/thread guards/query budget");
}
