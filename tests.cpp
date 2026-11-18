#include "vm.hpp"
#include <cassert>
#include <iostream>
int main(){vm::Config c;c.frames=2;c.tlbSize=2;c.policy="FIFO";vm::VirtualMemoryManager v(c);auto a=v.access(2050);assert(a.page==8&&a.offset==2&&a.pageFault);auto b=v.access(2051);assert(b.page==8&&b.tlbHit&&!b.pageFault);v.access(4096);auto d=v.access(8192);assert(d.pageFault);assert(v.stats().accesses==4);std::cout<<"All tests passed\n";}
