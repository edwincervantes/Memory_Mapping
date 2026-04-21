/*
 * memory_manager.c
 * Virtual Memory Manager - Project 1-1
 *
 * Translates logical addresses to physical addresses using a TLB and page table.
 * Implements demand paging with a BACKING_STORE.bin file.

 * ============================================================
 *  WORK SPLIT GUIDE
 * ============================================================
 *  Edwin owns: TLB logic, address translation pipeline,
 *                  statistics reporting, and Phase 2 page replacement
 *
 *  Sean owns: Page table, physical memory / backing store I/O,
 *                  page fault handler, and main() / output formatting
 * 
* ============================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ── Phase Selection ─────────────────────────────────────── */
/* Uncomment the line below to enable Phase 2 (128 frames + FIFO replacement) */
#define USE_PHASE2 

/* ── Constants ───────────────────────────────────────────── */
#define PAGE_TABLE_SIZE   256       /* 2^8 entries                  */
#define TLB_SIZE           16       /* 16 TLB entries               */
#define PAGE_SIZE         256       /* 256 bytes per page           */
#define FRAME_SIZE        256       /* 256 bytes per frame          */

#ifdef USE_PHASE2
#define NUM_FRAMES        128       /* Phase 2: 128 frames          */
#else
#define NUM_FRAMES        256       /* Phase 1: 256 frames          */
#endif

#define PHYS_MEM_SIZE  (FRAME_SIZE * NUM_FRAMES)  /* Physical memory size */
#define ADDRESS_MASK   0xFFFF       /* mask to 16-bit address       */
#define PAGE_MASK      0xFF00       /* upper 8 bits = page number   */
#define OFFSET_MASK    0x00FF       /* lower 8 bits = offset        */

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

#ifdef USE_PHASE2
/* FIFO queue for page replacement - tracks page number in each frame */
int frame_to_page[NUM_FRAMES];     /* Maps frame -> page number occupying it */
int fifo_queue[NUM_FRAMES];        /* FIFO queue of frame numbers            */
int fifo_head = 0;                 /* Front of queue (next to evict)         */
int fifo_tail = 0;                 /* Back of queue (next insertion point)   */
int fifo_count = 0;                /* Number of frames in use                */
#endif

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


/**
 * init_structures()
 * Initialize the TLB, page table, and physical memory to empty/invalid state.
 *
 *   1. Loop over tlb[] and set every entry's page_number to -1 (empty).
 *   2. Loop over page_table[] and set every entry's frame_number to -1
 *      and valid to 0.
 *   3. Optionally zero out physical_memory[] with memset.
 */
void init_structures(void) {
    for (int i = 0; i < TLB_SIZE; i++) {
        tlb[i].page_number = -1;
    }
    for (int i = 0; i < PAGE_TABLE_SIZE; i++) {
        page_table[i].frame_number = -1;
        page_table[i].valid = 0;
    }
    memset(physical_memory, 0, PHYS_MEM_SIZE);

#ifdef USE_PHASE2
    /* Initialize FIFO queue and frame-to-page mapping */
    for (int i = 0; i < NUM_FRAMES; i++) {
        frame_to_page[i] = -1;
        fifo_queue[i] = -1;
    }
    fifo_head = 0;
    fifo_tail = 0;
    fifo_count = 0;
#endif
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
    backing_store = fopen(filename, "rb");
    if (backing_store == NULL) {
        fprintf(stderr, "Error opening %s\n", filename);
        exit(1);
    }
}


/**
 * tlb_lookup()
 * Search the TLB for a given page number.
*
 *   1. Iterate through tlb[].
 *   2. If an entry's page_number matches, increment tlb_hits and
 *      return the corresponding frame_number.
 *   3. If no match is found, return -1 (TLB miss).
 *
 * @param page_number  The page number to look up.
 * @return             Frame number on hit, -1 on miss.
 */
int tlb_lookup(int page_number) {
    /* Iterate through all TLB entries looking for a match */
    for (int i = 0; i < TLB_SIZE; i++) {
        if (tlb[i].page_number == page_number) {
            /* TLB hit - increment counter and return frame number */
            tlb_hits++;
            return tlb[i].frame_number;
        }
    }
    /* TLB miss - page not found in TLB */
    return -1;
}

/**
 * tlb_update()
 * Insert or update a TLB entry using FIFO replacement.
 *
 *   1. Write the new (page_number, frame_number) pair into
 *      tlb[tlb_next_slot].
 *   2. Advance tlb_next_slot with wrap-around (mod TLB_SIZE).
 *
 * @param page_number   Page number to insert.
 * @param frame_number  Corresponding frame number.
 */
void tlb_update(int page_number, int frame_number) {
    /* Insert new entry at the current FIFO slot */
    tlb[tlb_next_slot].page_number = page_number;
    tlb[tlb_next_slot].frame_number = frame_number;

    /* Advance to next slot with wrap-around */
    tlb_next_slot = (tlb_next_slot + 1) % TLB_SIZE;
}

#ifdef USE_PHASE2
/**
 * tlb_remove()
 * Remove a page from the TLB when it is evicted from physical memory.
 *
 * @param page_number  The page number to remove from TLB.
 */
void tlb_remove(int page_number) {
    for (int i = 0; i < TLB_SIZE; i++) {
        if (tlb[i].page_number == page_number) {
            tlb[i].page_number = -1;
            tlb[i].frame_number = -1;
            return;
        }
    }
}
#endif


/**
 * handle_page_fault()
 * Load a page from the backing store into physical memory.
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
    page_faults++;

    int frame;

#ifdef USE_PHASE2
    /* Phase 2: Use FIFO replacement when all frames are occupied */
    if (fifo_count < NUM_FRAMES) {
        /* Free frames available - use next free frame */
        frame = next_free_frame;
        next_free_frame++;
        fifo_count++;
    } else {
        /* No free frames - evict the oldest page (FIFO) */
        frame = fifo_queue[fifo_head];
        int evicted_page = frame_to_page[frame];

        /* Invalidate the evicted page in the page table */
        page_table[evicted_page].frame_number = -1;
        page_table[evicted_page].valid = 0;

        /* Remove evicted page from TLB if present */
        tlb_remove(evicted_page);

        /* Advance FIFO head */
        fifo_head = (fifo_head + 1) % NUM_FRAMES;
    }

    /* Add this frame to the back of the FIFO queue */
    fifo_queue[fifo_tail] = frame;
    fifo_tail = (fifo_tail + 1) % NUM_FRAMES;

    /* Update frame-to-page mapping */
    frame_to_page[frame] = page_number;
#else
    /* Phase 1: Simple allocation (no replacement needed) */
    frame = next_free_frame;
    next_free_frame++;
#endif

    /* Load the page from backing store into physical memory */
    fseek(backing_store, page_number * PAGE_SIZE, SEEK_SET);
    fread(&physical_memory[frame * FRAME_SIZE], sizeof(signed char), PAGE_SIZE, backing_store);

    /* Update page table */
    page_table[page_number].frame_number = frame;
    page_table[page_number].valid = 1;

    return frame;
}

/**
 * translate_address()
 * Translate a logical address to a physical address and retrieve the byte.
 *
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
    /* 1. Mask the logical address to 16 bits */
    int masked_address = logical_address & ADDRESS_MASK;

    /* 2. Extract the page number (upper 8 bits) */
    int page_number = (masked_address >> 8) & 0xFF;

    /* 3. Extract the page offset (lower 8 bits) */
    int offset = masked_address & 0xFF;

    int frame_number;

    /* 4. Try TLB lookup first */
    frame_number = tlb_lookup(page_number);

    if (frame_number == -1) {
        /* TLB miss - check the page table */
        if (page_table[page_number].valid == 1) {
            /* Page is in memory - get frame from page table */
            frame_number = page_table[page_number].frame_number;
        } else {
            /* Page fault - load page from backing store */
            frame_number = handle_page_fault(page_number);
            /* Update TLB with the new mapping */
            tlb_update(page_number, frame_number);
        }
    }

    /* 5. Compute physical address */
    int physical_address = frame_number * FRAME_SIZE + offset;

    /* 6. Read the signed byte from physical memory */
    signed char value = physical_memory[physical_address];

    /* 7. Print output line (hex format) */
    printf("0x%04x -> 0x%04x: %d\n",
           masked_address, physical_address, value);

    /* 8. Increment total addresses counter */
    total_addresses++;
}


/**
 * print_statistics()
 * Print TLB hit rate and page-fault rate after all addresses are processed.
 *
 *   1. Compute TLB hit rate   = (tlb_hits   / (double)total_addresses) * 100
 *   2. Compute page fault rate = (page_faults / (double)total_addresses) * 100
 *   3. Print both as percentages with at least 2 decimal places.
 *      Example format:
 *         "TLB Hit Rate: 5.60%"
 *         "Page Fault Rate: 24.40%"
 */
void print_statistics(void) {
    /* Compute TLB hit rate as a percentage */
    double tlb_hit_rate = (tlb_hits / (double)total_addresses) * 100.0;

    /* Compute page fault rate as a percentage */
    double page_fault_rate = (page_faults / (double)total_addresses) * 100.0;

    /* Print statistics with 2 decimal places */
    printf("Page-fault rate: %.2f%%\n", page_fault_rate);
    printf("TLB hit rate: %.2f%%\n", tlb_hit_rate);
}


/**
 * main()
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
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <address_file>\n", argv[0]);
        exit(1);
    }

    init_structures();
    open_backing_store("BACKING_STORE.bin");

    FILE *address_file = fopen(argv[1], "r");
    if (address_file == NULL) {
        fprintf(stderr, "Error opening %s\n", argv[1]);
        fclose(backing_store);
        exit(1);
    }

    int logical_address;
    while (fscanf(address_file, "%d", &logical_address) == 1) {
        translate_address(logical_address);
    }

    print_statistics();

    fclose(address_file);
    fclose(backing_store);

    return 0;
}

