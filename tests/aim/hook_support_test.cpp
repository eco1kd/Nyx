#include "features/aim/hook_support.hpp"
#include <cassert>
#include <iostream>
#include <unistd.h>
using namespace lemming::aim::hook_support;
int main(){
 constexpr Address site=0x50000000,span=0x8000000;
 assert(cameraSlotMatches(0x50010000,0x50010000,0x60000000,true,0x50000000,0x50100000));
 assert(!cameraSlotMatches(0x50010004,0x50010000,0x60000000,true,0x50000000,0x50100000));
 assert(!cameraSlotMatches(0x50010000,0x50010000,0,false,0x50000000,0x50100000));
 assert(!cameraSlotMatches(0x50010000,0x50010000,0x60000000,false,0x50000000,0x50100000));
 assert(!cameraSlotMatches(0x50010000,0x50010000,0x60000000,true,0x50100000,0x50200000));
 const Address adjacentWords[]={17,0x60004000};assert(adjacentWords[0]!=0x50010000&&adjacentWords[1]!=0x50010000);
 assert(cameraSlotMatches(0x50010000,0x50010000,reinterpret_cast<Address>(adjacentWords),true,0x50000000,0x50100000));
 assert(branchFits(site,site-span));assert(branchFits(site,site+span-4));assert(!branchFits(site,site+span));assert(!branchFits(site,site-span-4));assert(!branchFits(site,site+2));assert(!branchFits(0,0));
 std::uint32_t word=0;assert(branchWord(0x973a9e0,0x9739fa4,word)&&word==0x97fffd71);
 assert(branchWord(site,site-span,word)&&word==0x96000000);assert(branchWord(site,site+span-4,word)&&word==0x95ffffff);
 assert(!branchWord(site,site+span,word));
 std::vector<Mapping> maps{{site-100*0x100000,site+90*0x100000}};
 for(Address page:{Address(4096),Address(16384),Address(65536)}){
  auto candidates=relayCandidates(site,page,maps);assert(!candidates.empty());
  for(Address p:candidates){assert(!(p&(page-1)));assert(branchFits(site,p));assert(p+page<=maps[0].begin||p>=maps[0].end);assert((p>=site?p-site:site-p)>64*0x100000);}
 }
 assert(relayCandidates(site,0,maps).empty());assert(relayCandidates(site,12288,maps).empty());assert(relayCandidates(site,4096,{}).empty());
 assert(relayCandidates(site,4096,{{site-span,site+span}}).empty());
 auto candidates=relayCandidates(site,16384,{{site+0x9000,site+0x19000},{site-0x19000,site+0x1000}});
 for(Address p:candidates)for(auto m:std::vector<Mapping>{{site+0x9000,site+0x19000},{site-0x19000,site+0x1000}})assert(p+16384<=m.begin||p>=m.end);
 Address page=Address(sysconf(_SC_PAGESIZE));assert(pageSizeValid(page));
 void* anchor=mmap(nullptr,page,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);assert(anchor!=MAP_FAILED);
 *static_cast<unsigned char*>(anchor)=0xa5;AllocationStats stats{};void* relay=allocateNear(reinterpret_cast<Address>(anchor),page,stats);
 assert(relay!=MAP_FAILED&&relay!=anchor&&branchFits(reinterpret_cast<Address>(anchor),reinterpret_cast<Address>(relay)));
 assert(*static_cast<unsigned char*>(anchor)==0xa5);assert(stats.candidates&&stats.attempts);
 munmap(relay,page);munmap(anchor,page);
 std::cout<<"PASS exact native camera-slot validation with non-plaintext metadata, BL limits/opcodes, 4K/16K/64K free-gap search beyond old hints, and real non-clobbering near allocation\n";
}
