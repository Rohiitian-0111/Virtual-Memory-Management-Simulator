#include "vm.hpp"
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
using namespace vm;
int main(int argc,char**argv){Config c;std::string file="data/addresses.txt";for(int i=1;i<argc;i++){std::string a=argv[i];if(a=="--policy"&&i+1<argc)c.policy=argv[++i];else if(a=="--frames"&&i+1<argc)c.frames=std::stoul(argv[++i]);else if(a=="--tlb"&&i+1<argc)c.tlbSize=std::stoul(argv[++i]);else if(a=="--file"&&i+1<argc)file=argv[++i];}
try{VirtualMemoryManager v(c);std::ifstream in(file);if(!in){std::cerr<<"Cannot open "<<file<<"\n";return 1;}std::cout<<"Policy: "<<c.policy<<" | Frames: "<<c.frames<<" | TLB: "<<c.tlbSize<<"\n\n";uint32_t a;while(in>>a){auto r=v.access(a);std::cout<<"VA "<<std::setw(5)<<r.virtualAddress<<" | page "<<std::setw(3)<<r.page<<" off "<<std::setw(3)<<r.offset<<" | frame "<<std::setw(2)<<r.frame<<" | PA "<<std::setw(5)<<r.physicalAddress<<" | "<<(r.tlbHit?"TLB HIT ":"TLB MISS")<<(r.pageFault?" | PAGE FAULT":"")<<(r.evictedPage>=0?" | evicted page "+std::to_string(r.evictedPage):"")<<"\n";}auto&s=v.stats();std::cout<<"\n=== Statistics ===\nAccesses: "<<s.accesses<<"\nTLB hit rate: "<<std::fixed<<std::setprecision(2)<<s.tlbHitRate()<<"%\nPage fault rate: "<<s.faultRate()<<"%\nAverage access time: "<<s.amat()<<" ns\n";}catch(const std::exception&e){std::cerr<<e.what()<<"\n";return 2;}}
