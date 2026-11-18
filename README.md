# Virtual Memory Management Simulator

A portfolio-ready **C++17 Operating Systems project** that models virtual-to-physical address translation with a page table, TLB, demand paging, simulated backing storage, and FIFO/LRU/Clock page replacement. It also includes a lightweight browser visualizer.

## Highlights

- 16-bit virtual address space with configurable power-of-two page size
- Bitwise page-number and offset extraction
- Page table with valid, frame, reference and recency metadata
- LRU Translation Lookaside Buffer with invalidation on page eviction
- Demand paging with deterministic simulated backing-store bytes
- FIFO, LRU and Clock physical-frame replacement
- TLB hit rate, page-fault rate and average memory-access-time metrics
- CLI trace runner plus interactive web visualization
- CMake build and smoke tests

## Translation pipeline

```text
Virtual Address
      |
      v
 Page + Offset
      |
      v
     TLB ---- hit --------------------+
      |                               |
     miss                             |
      v                               |
 Page Table                           |
      |                               |
 invalid? -- no ----------------------+ 
      |
     yes
      v
 Page Fault -> Backing Store -> Replacement
      |                               |
      +----------> Physical Frame <---+
                         |
                         v
                  Physical Address
```

## Build and run

```bash
cmake -S . -B build
cmake --build build
./build/vm_simulator --policy LRU --frames 8 --tlb 4 --file data/addresses.txt
```

Windows PowerShell after building with Visual Studio may use `./build/Debug/vm_simulator.exe`.

Policies: `FIFO`, `LRU`, `CLOCK`.

Run tests:

```bash
ctest --test-dir build --output-on-failure
```

## Web visualizer

No framework or installation is required. Open `web/index.html` in a browser. Enter virtual addresses and switch among LRU/FIFO/CLOCK to see TLB contents, physical frames, page faults, evictions and live metrics.

The web version mirrors the educational behavior of the C++ engine; the C++ implementation remains the primary simulator.

## Important implementation idea

With a 256-byte page, the lower 8 bits are the offset and the remaining bits identify the virtual page:

```cpp
page   = virtualAddress >> 8;
offset = virtualAddress & 0xFF;
```

On every access the simulator checks the TLB first. A TLB miss triggers a page-table lookup. If the page-table entry is invalid, a page fault loads the page into a free frame or selects a victim using the configured replacement policy. Evicting a page also invalidates its page-table and TLB entries.

## Complexity

- TLB lookup: average O(1)
- Page-table lookup: O(1)
- FIFO replacement: O(1)
- Clock replacement: amortized scan over frames
- LRU victim selection in this educational implementation: O(number of frames)
