/*
 * memory_manager.c
 * Virtual Memory Manager - Project 1-1
 *
 * Translates logical addresses to physical addresses using a TLB and page table.
 * Implements demand paging with a BACKING_STORE.bin file.

 * ============================================================
 *  WORK SPLIT GUIDE
 * ============================================================
 *  Partner A owns: TLB logic, address translation pipeline,
 *                  statistics reporting, and Phase 2 page replacement
 *
 *  Partner B owns: Page table, physical memory / backing store I/O,
 *                  page fault handler, and main() / output formatting
 * 
* ============================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ── Constants ───────────────────────────────────────────── */
#define PAGE_TABLE_SIZE   256       /* 2^8 entries                  */
#define TLB_SIZE           16       /* 16 TLB entries               */
#define PAGE_SIZE         256       /* 256 bytes per page           */
#define FRAME_SIZE        256       /* 256 bytes per frame          */
#define NUM_FRAMES        256       /* Phase 1: 256 frames          */
#define PHYS_MEM_SIZE  (FRAME_SIZE * NUM_FRAMES)  /* 65,536 bytes   */
#define ADDRESS_MASK   0xFFFF       /* mask to 16-bit address       */
#define PAGE_MASK      0xFF00       /* upper 8 bits = page number   */
#define OFFSET_MASK    0x00FF       /* lower 8 bits = offset        */

/* Phase 2 constant – change NUM_FRAMES to this value for part 2   */
#define NUM_FRAMES_PHASE2 128

/* ── Data Structures ─────────────────────────────────────── */

/* TLB entry – maps a page number to a frame number */
typedef struct {
    int page_number;   /* -1 means this slot is empty */
    int frame_number;
} TLBEntry;

/* Page table entry */
typedef struct {
    int frame_number;  /* -1 means page is NOT in memory */
    int valid;         /* 1 = in memory, 0 = not loaded  */
} PageTableEntry;

/* ── Global State ────────────────────────────────────────── */

/* Physical memory – a flat byte array */
signed char physical_memory[PHYS_MEM_SIZE];

/* TLB – 16 slots, managed with FIFO replacement */
TLBEntry tlb[TLB_SIZE];

/* Page table – one entry per virtual page */
PageTableEntry page_table[PAGE_TABLE_SIZE];

/* Next free frame to allocate (Phase 1: simple counter) */
int next_free_frame = 0;

/* TLB FIFO index – points to the slot to overwrite next */
int tlb_next_slot = 0;

/* Backing store file handle */
FILE *backing_store;

/* ── Statistics counters ─────────────────────────────────── */
int total_addresses = 0;
int tlb_hits        = 0;
int page_faults     = 0;

/* ═══════════════════════════════════════════════════════════
 *  PARTNER B – Initialization
 * ═══════════════════════════════════════════════════════════ */

/**
 * init_structures()
 * Initialize the TLB, page table, and physical memory to empty/invalid state.
 *
 * [PARTNER B] TODO:
 *   1. Loop over tlb[] and set every entry's page_number to -1 (empty).
 *   2. Loop over page_table[] and set every entry's frame_number to -1
 *      and valid to 0.
 *   3. Optionally zero out physical_memory[] with memset.
 */
void init_structures(void) {
    /* TODO [PARTNER B]: implement initialization */
}

/**
 * open_backing_store()
 * Open BACKING_STORE.bin for random-access reading.
 *
 * [PARTNER B] TODO:
 *   1. Use fopen() in "rb" mode.
 *   2. If fopen() returns NULL, print an error and exit(1).
 */
void open_backing_store(const char *filename) {
    /* TODO [PARTNER B]: open the file and store the handle in backing_store */
}

/* ═══════════════════════════════════════════════════════════
 *  PARTNER A – TLB Management
 * ═══════════════════════════════════════════════════════════ */

/**
 * tlb_lookup()
 * Search the TLB for a given page number.
 *
 * [PARTNER A] TODO:
 *   1. Iterate through tlb[].
 *   2. If an entry's page_number matches, increment tlb_hits and
 *      return the corresponding frame_number.
 *   3. If no match is found, return -1 (TLB miss).
 *
 * @param page_number  The page number to look up.
 * @return             Frame number on hit, -1 on miss.
 */
int tlb_lookup(int page_number) {
    /* TODO [PARTNER A]: implement TLB search */
    return -1; /* placeholder */
}

/**
 * tlb_update()
 * Insert or update a TLB entry using FIFO replacement.
 *
 * [PARTNER A] TODO:
 *   1. Write the new (page_number, frame_number) pair into
 *      tlb[tlb_next_slot].
 *   2. Advance tlb_next_slot with wrap-around (mod TLB_SIZE).
 *
 * @param page_number   Page number to insert.
 * @param frame_number  Corresponding frame number.
 */
void tlb_update(int page_number, int frame_number) {
    /* TODO [PARTNER A]: implement FIFO TLB insertion */
}

/* ═══════════════════════════════════════════════════════════
 *  PARTNER B – Page Fault Handler & Physical Memory
 * ═══════════════════════════════════════════════════════════ */

/**
 * handle_page_fault()
 * Load a page from the backing store into physical memory.
 *
 * [PARTNER B] TODO:
 *   1. Increment the page_faults counter.
 *   2. Determine the target frame (use next_free_frame for Phase 1).
 *      Increment next_free_frame after allocation.
 *   3. Seek to the correct position in backing_store:
 *         position = page_number * PAGE_SIZE
 *      Use fseek() with SEEK_SET.
 *   4. Read PAGE_SIZE bytes from backing_store into the correct
 *      location in physical_memory:
 *         &physical_memory[frame * FRAME_SIZE]
 *      Use fread().
 *   5. Update page_table[page_number]: set frame_number and valid = 1.
 *   6. Return the newly allocated frame number.
 *
 * @param page_number  The faulting page number.
 * @return             The frame number it was loaded into.
 */
int handle_page_fault(int page_number) {
    /* TODO [PARTNER B]: implement demand paging */
    return -1; /* placeholder */
}

/* ═══════════════════════════════════════════════════════════
 *  PARTNER A – Address Translation
 * ═══════════════════════════════════════════════════════════ */

/**
 * translate_address()
 * Translate a logical address to a physical address and retrieve the byte.
 *
 * [PARTNER A] TODO – wire the pipeline together:
 *   1. Mask the logical address to 16 bits (ADDRESS_MASK).
 *   2. Extract the page number  : (masked >> 8) & 0xFF
 *   3. Extract the page offset  : masked & 0xFF
 *   4. Call tlb_lookup(page_number).
 *      - Hit  → use the returned frame number.
 *      - Miss → check page_table[page_number].valid:
 *                  * valid == 1 → use page_table[page_number].frame_number
 *                  * valid == 0 → call handle_page_fault(page_number)
 *               After resolving, call tlb_update() with the frame number.
 *   5. Compute physical address: frame_number * FRAME_SIZE + offset
 *   6. Read the signed byte from physical_memory[physical_address].
 *   7. Print output line:
 *         "Virtual address: %d  Physical address: %d  Value: %d\n"
 *      using the original (unmasked) logical address, physical address,
 *      and the signed byte value.
 *   8. Increment total_addresses.
 *
 * @param logical_address  The raw integer read from addresses.txt.
 */
void translate_address(int logical_address) {
    /* TODO [PARTNER A]: implement full translation pipeline */
}

/* ═══════════════════════════════════════════════════════════
 *  PARTNER A – Statistics
 * ═══════════════════════════════════════════════════════════ */

/**
 * print_statistics()
 * Print TLB hit rate and page-fault rate after all addresses are processed.
 *
 * [PARTNER A] TODO:
 *   1. Compute TLB hit rate   = (tlb_hits   / (double)total_addresses) * 100
 *   2. Compute page fault rate = (page_faults / (double)total_addresses) * 100
 *   3. Print both as percentages with at least 2 decimal places.
 *      Example format:
 *         "TLB Hit Rate: 5.60%"
 *         "Page Fault Rate: 24.40%"
 */
void print_statistics(void) {
    /* TODO [PARTNER A]: compute and print statistics */
}

/* ═══════════════════════════════════════════════════════════
 *  PARTNER B – main()
 * ═══════════════════════════════════════════════════════════ */

/**
 * main()
 *
 * [PARTNER B] TODO:
 *   1. Validate command-line arguments (expect exactly 1: the address file).
 *      Print usage and exit(1) if missing.
 *   2. Call init_structures().
 *   3. Call open_backing_store("BACKING_STORE.bin").
 *   4. Open addresses.txt (argv[1]) for reading; exit on error.
 *   5. Read each integer line-by-line (use fscanf or fgets+sscanf).
 *   6. For each integer, call translate_address().
 *   7. After all addresses, call print_statistics().
 *   8. Close both open files.
 *   9. Return 0.
 */
int main(int argc, char *argv[]) {
    /* TODO [PARTNER B]: implement main */
    return 0;
}

/* ═══════════════════════════════════════════════════════════
 *  PHASE 2 – Page Replacement  [PARTNER A]
 * ═══════════════════════════════════════════════════════════
 *
 * Phase 2 reduces physical memory to 128 frames (NUM_FRAMES_PHASE2).
 * When all frames are occupied, a page-replacement policy is needed.
 *
 * [PARTNER A] TODO:
 *   1. Add a "frame free list" or occupancy tracker (e.g., a boolean
 *      array free_frames[NUM_FRAMES_PHASE2]).
 *   2. Implement FIFO or LRU replacement:
 *
 *      FIFO: maintain a queue of frame numbers in allocation order.
 *            When no free frame exists, evict the front of the queue,
 *            invalidate its page_table entry (valid=0, frame=-1),
 *            and also remove it from the TLB if present.
 *
 *      LRU:  maintain a "last used" timestamp per frame (increment a
 *            global clock on every access). Evict the frame with the
 *            smallest timestamp.
 *
 *   3. Modify handle_page_fault() to use this logic when
 *      next_free_frame >= NUM_FRAMES_PHASE2.
 *
 *   Tip: isolate this behind a #define USE_PHASE2 guard so Phase 1
 *   still compiles and passes tests independently.
 */
