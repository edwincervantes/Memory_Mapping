# Authors

### Sean Linko
### Edwin Cervantes

# Memory Mapping

A virtual memory manager that translates logical addresses to physical addresses using a TLB (Translation Lookaside Buffer) and page table. Implements demand paging with a backing store.

## Files

| File | Description |
|------|-------------|
| `memory_manager.c` | Main program that simulates virtual memory address translation. Implements a 16-entry TLB with FIFO replacement, a 256-entry page table, and handles page faults by loading pages from the backing store. |
| `addresses.txt` | Input file containing logical (virtual) addresses to translate. Each line contains one 32-bit integer address. |
| `BACKING_STORE.bin` | A binary file representing secondary storage (disk). Contains 256 pages of 256 bytes each (65,536 bytes total). Pages are loaded from here into physical memory on demand. |
| `correct.txt` | Expected output for verification. Compare your output against this file to validate correctness. |

## How to Compile

```bash
gcc memory_manager.c -o memory_manager
```

## How to Run

```bash
./memory_manager addresses.txt
```

## Expected Output

For each address in `addresses.txt`, the program outputs:
```
0x<virtual_addr> -> 0x<physical_addr>: <signed_byte>
```

After processing all addresses, it prints statistics:
```
Page-fault rate: X.XX%
TLB hit rate: X.XX%
```

## Verifying Output

To check if your output matches the expected results:
```bash
./memory_manager addresses.txt > output.txt
diff output.txt correct.txt
```

## Phase 2: Page Replacement

Phase 2 reduces physical memory from 256 frames to 128 frames, requiring FIFO page replacement when memory is full.

### Enabling Phase 2

**Option 1:** Edit `memory_manager.c` and uncomment the `USE_PHASE2` definition at the top:
```c
#define USE_PHASE2
```

**Option 2:** Compile with the flag:
```bash
gcc -DUSE_PHASE2 memory_manager.c -o memory_manager
```

### Expected Results

| Phase | Frames | Page-fault rate | TLB hit rate |
|-------|--------|-----------------|--------------|
| 1     | 256    | 24.90%          | 5.50%        |
| 2     | 128    | 53.60%          | 4.80%        |

Phase 2 has a higher page-fault rate because pages must be evicted and reloaded more frequently with limited memory.
