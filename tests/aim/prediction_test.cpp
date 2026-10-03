#include "features/aim/runtime.hpp"
#include <cassert>
#include <cstdio>
#include <limits>
#include <cstring>
extern "C" int __android_log_print(int,const char*,const char*,...){return 0;}
using namespace lemming::aim;namespace rt=runtime;
static int originalCalls=0;static void original(void*,rt::ExecuteTime,const void*){++originalCalls;}
static bool near(float a,float b){return std::fabs(a-b)<.002f;}
int main(){
 prediction::Motion m;Vec3 point{0,0,10};assert(!m.update({0,0,0},1,2,1));assert(m.update({.1f,0,0},1.02,2,1));assert(near(m.velocity.x,5));auto p=m.lead(point,1.02,20,.12f);assert(near(p.x,.7f));assert(m.update({.1f,0,0},1.02,2,1)&&near(m.velocity.x,5));assert(m.lead(point,1.2,20,.12f).x==0);assert(m.lead(point,1.02,20,.12f,false).x==0);assert(m.lead(point,.9,20,.12f).x==0);
 assert(m.update({.1f,0,0},1.04,2,1));assert(near(m.velocity.x,0));m.update({.2f,0,0},1.06,2,1);m.update({.1f,0,0},1.08,2,1);assert(m.velocity.x<0);assert(!m.update({20,0,0},1.10,2,1)&&m.velocity.x==0);assert(!m.update({20,0,0},1.12,3,1));assert(!m.update({20,0,0},1.14,3,2));assert(!m.update({20,0,0},2,3,2));assert(!m.update({std::numeric_limits<float>::quiet_NaN(),0,0},2.1,3,2)&&m.samples==0);
 m.update({0,0,0},1,2,1);m.update({.5f,0,0},1.02,2,1);assert(near(m.lead(point,1.02,100,1).x,1));assert(m.lead(point,1.02,std::numeric_limits<float>::infinity(),1).x==0);
 assert(!prediction::cameraLease(true,true,false,false,true,100,180,1,2,1,2));assert(!prediction::cameraLease(true,true,true,false,false,100,180,1,2,1,2));assert(prediction::cameraLease(true,true,true,false,true,100,180,1,2,1,2));assert(!prediction::cameraLease(true,true,true,true,true,100,180,1,2,1,2));assert(!prediction::cameraLease(true,true,true,false,true,181,180,1,2,1,2));assert(!prediction::cameraLease(true,true,true,false,true,100,180,1,2,3,2));assert(!prediction::cameraLease(false,true,true,false,true,100,180,1,2,1,2));
 prctl(PR_SET_NAME,"UnityMain",0,0,0);Configuration c;c.normal.enabled=true;c.autoFire.enabled=c.autoFire.returnCamera=c.autoFire.autoScope=true;publish(c);g_menuOpen=false;rt::g_functionsValid=true;rt::g_cameraOriginal.store(original);g_inputHookReady=false;g_silentHookReady=false;g_visibilityReady=false;debug::normalReason=0;rt::cameraTick(nullptr,{1,.016f},nullptr);assert(originalCalls==1&&debug::normalReason==7);g_inputHookReady=g_silentHookReady=g_visibilityReady=true;rt::cameraTick(nullptr,{2,.016f},nullptr);assert(originalCalls==2&&debug::normalReason==7);
 alignas(8) unsigned char camera[64]{},owner[32]{},world[32]{};void* o=owner;void* w=world;std::memcpy(camera+0x18,&o,8);std::memcpy(camera+0x20,&w,8);rt::ed::refreshMaps(true);rt::g_savedAutoView.valid=true;rt::g_cameraLeasePlayer=reinterpret_cast<std::uintptr_t>(owner);rt::g_cameraLeaseWorld=reinterpret_cast<std::uintptr_t>(world);rt::g_cameraLeaseUntil=rt::autoNow()+80;assert(rt::automationOwnsCamera(camera,c));rt::cameraTick(camera,{3,.016f},nullptr);assert(originalCalls==3&&debug::normalReason==17);rt::g_cameraLeaseUntil=rt::autoNow()-1;assert(!rt::automationOwnsCamera(camera,c));c.autoFire.enabled=false;assert(!rt::automationOwnsCamera(camera,c));c.normal.predictionMs=std::numeric_limits<float>::quiet_NaN();publish(c);assert(configuration().normal.predictionMs==20);

 prediction::Motion movement;Angles plain{},predicted{};Settings aimSettings;float time=3;
 for(int i=0;i<200;++i){time+=.016f;Vec3 target{4*(time-3),0,40};movement.update(target,time,2,1);plain=smooth(plain,anglesFor(target),.12f,.016f);predicted=smooth(predicted,anglesFor(movement.lead(target,time,20,.12f)),.12f,.016f);}
 auto real=anglesFor({4*(time-3),0,40});assert(std::fabs(wrap(real.yaw-predicted.yaw))<std::fabs(wrap(real.yaw-plain.yaw))*.5f);
 puts("PASS velocity lead, duplicate snapshots, stops/reversals, stale/teleport/world/team/NaN resets, 1m cap, camera ownership expiry and real camera callback fallback with all automation toggles enabled");
}
