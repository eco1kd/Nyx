#include <cassert>
#include <cstdio>
#include <limits>
#include "features/aim/core.hpp"
#include "features/aim/assists.hpp"
using namespace lemming::aim;
int main(){namespace a=automation;a::Options cfg{};assert(!cfg.enabled&&!cfg.returnCamera&&!cfg.autoScope);cfg.enabled=true;cfg.returnCamera=true;cfg.autoScope=true;cfg.delayMs=30;
 a::Observation o{};o.safe=o.target=true;o.scopeSupported=true;o.player=1;o.world=2;o.weapon=3;o.targetId=4;a::Cycle c{};
 auto r=c.step(100,cfg,o);assert(r.save&&r.aim&&!r.attack&&!r.scopePulse);o.aligned=true;r=c.step(110,cfg,o);assert(r.scopePulse&&!r.attack&&c.stage==a::Stage::Scoping);r=c.step(120,cfg,o);assert(!r.scopePulse&&!r.attack);o.scoped=true;r=c.step(130,cfg,o);assert(!r.attack);r=c.step(161,cfg,o);assert(r.attack&&!r.restore);o.shot=true;o.target=false;r=c.step(170,cfg,o);assert(r.completed&&r.restore&&r.scopePulse&&!r.attack&&!r.aim);r=c.step(171,cfg,o);assert(!r.attack&&!r.aim);
 o.shot=false;o.target=true;o.scoped=false;o.aligned=false;c.reset();c.step(100,cfg,o);r=c.step(1001,cfg,o);assert(r.timedOut&&r.restore&&!r.attack);
 for(int event=0;event<6;++event){c.reset();o.safe=o.target=true;o.manual=false;o.weapon=3;c.step(100,cfg,o);auto x=o;auto k=cfg;if(event==0)k.enabled=false;if(event==1)x.safe=false;if(event==2)x.manual=true;if(event==3)x.weapon=5;if(event==4)x.target=false;if(event==5)k.returnCamera=false;r=c.step(110,k,x);assert(r.restore&&!r.attack&&!r.aim);}
 c.reset();c.step(100,cfg,o);r=c.step(std::numeric_limits<double>::quiet_NaN(),cfg,o);assert(r.timedOut&&r.restore&&!r.attack);
 cfg.returnCamera=false;cfg.autoScope=false;cfg.delayMs=0;o.aligned=true;c.reset();r=c.step(100,cfg,o);assert(r.attack&&!r.save&&!r.aim);o.manual=true;r=c.step(110,cfg,o);assert(!r.restore&&!r.attack);o.manual=false;
 cfg.autoScope=true;o.scoped=true;c.reset();r=c.step(100,cfg,o);assert(r.attack&&!r.scopePulse);o.shot=true;r=c.step(110,cfg,o);assert(r.completed&&!r.scopePulse);o.shot=false;
 auto bad=cfg;bad.delayMs=std::numeric_limits<float>::infinity();bad.intervalMs=-1;bad.timeoutMs=10000;bad=a::sanitize(bad);assert(bad.delayMs==0&&bad.intervalMs==30&&bad.timeoutMs==2000);
 Configuration defaults;assert(!defaults.normal.onAttack&&!defaults.silent.onAttack);Settings settings;settings.fov=1000;settings=sanitize(settings);assert(settings.fov==180);Selection rear{};HitVolume v{{0,0,-10},{.2f,.2f,.2f},10};consider(rear,settings,4,100,{0,0,0},{0,0,1},v);assert(rear.valid&&rear.angle==180);settings.fov=179;rear={};consider(rear,settings,4,100,{0,0,0},{0,0,1},v);assert(!rear.valid);assert(assists::fovGeometry(180,1920,1080,1,1).coversScreen);
 puts("PASS 360 rear targets, immediate-aim defaults, shot acknowledgement, delay, scope preservation, cancellation, timeout, invalid clock and manual override");}
