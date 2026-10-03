#include "features/aim/native_pair.hpp"
#include <cassert>
#include <cstdio>
#include <map>
using namespace lemming::aim::native_pair;
struct Ops {std::map<std::uintptr_t,std::uint32_t> words{{0x1004,11},{0x3004,22}};unsigned writes=0,rw=0,protect=0,flushed=0;unsigned denyRw=0,denyRx=0;bool corrupt=false;
 bool read(std::uintptr_t p,std::uint32_t& v){v=words[p];return true;}
 bool rwx(std::uintptr_t,std::uintptr_t){return ++rw!=denyRw;}
 bool rx(std::uintptr_t,std::uintptr_t){return ++protect!=denyRx;}
 void write(std::uintptr_t p,std::uint32_t v){++writes;if(corrupt&&writes==1)++v;words[p]=v;}
 void flush(std::uintptr_t){++flushed;}
};
int main(){std::array<std::uintptr_t,2> sites{0x1004,0x3004};std::array<std::uint32_t,2> old{11,22},replacement{33,44};
 Ops ok;assert(apply(ok,sites,old,replacement,4096)==Ready&&ok.words[sites[0]]==33&&ok.words[sites[1]]==44&&ok.flushed==2);
 Ops changed;changed.words[sites[1]]=55;assert(apply(changed,sites,old,replacement,4096)==Changed&&changed.writes==0&&changed.rw==0);
 Ops denied;denied.denyRw=2;assert(apply(denied,sites,old,replacement,4096)==WritableDenied&&denied.writes==0);
 Ops deniedProtect;deniedProtect.denyRw=1;deniedProtect.denyRx=1;assert(apply(deniedProtect,sites,old,replacement,4096)==RestoreDenied&&deniedProtect.writes==0);
 Ops bad;bad.corrupt=true;assert(apply(bad,sites,old,replacement,4096)==VerifyFailed&&bad.words[sites[0]]==11&&bad.words[sites[1]]==22&&bad.writes==4);
 Ops rx;rx.denyRx=1;assert(apply(rx,sites,old,replacement,4096)==RestoreDenied&&rx.words[sites[0]]==11&&rx.words[sites[1]]==22);
 Ops partial;partial.corrupt=true;partial.denyRw=3;assert(apply(partial,sites,old,replacement,4096)==RollbackFailed);
 Ops same;same.words[0x1008]=22;sites[1]=0x1008;assert(apply(same,sites,old,replacement,4096)==Ready&&same.rw==1&&same.protect==1);
 puts("PASS exact two-site transaction, changed/denied no-write, verify/RX rollback of BOTH words, rollback failure classification, same-page protection deduplication");
}
