struct page_table_entry {
	unsigned int p:1;			// page present flag
	unsigned int m:1;			// page modified flag
	unsigned int r:1;			// page referenced flag
	unsigned int o:1;			// page pagedout flag
	unsigned int frame:28;		// page frame index mapping
};
typedef struct page_table_entry pt_entry;
typedef pt_entry *pte_ptr;

struct page_queue_node {		// for the page replacement queue
	int page_num;
	int frame_num;
	pte_ptr *page_ptr;
	unsigned int cntr:32;
	struct page_queue_node *next_ptr;
};
typedef struct page_queue_node p_queue;
typedef p_queue *pq_ptr;

struct pager_ptrs {
	char *p_name;
	void (*add_page)(pq_ptr *, pq_ptr *, int, int);
	int (*get_victim)(pt_entry [], pq_ptr *, pq_ptr *, int *);
};
typedef struct pager_ptrs pager_type;
typedef pager_type *pager_ptr;

/* Read in the random number file. */
void getrands(FILE *, int[]);

/* Read a single line from a file. Discard lines not comprising 2 integers.
Record read/write bit and page number: caller must check for EOF */
int file_readline(FILE *, int *, int *, int*);

/* Set all elements of frametable to -1 for bounds checking. */
void initialize_frametable(int []);

/* Print out the virutal frames mapped to physical frames, if present. */
void print_frametable(int [], int);

/* Fill the frame and page tables when empty frames are available.
	Start tracking mapped, zeroed and page-in counts. */ 
void fill_tables(pt_entry [], int [], int, int, int *, long *, long *, long*);

/* Print the pagetable read, modified, and pagedout status. */
void print_pagetable(pt_entry []);

/* Print the optional per-instruction output. */
void print_output(pt_entry [], int, int, int, int, int);

/* Check to see if the paging queue is empty. */
int is_empty (pq_ptr);

/* Print the paging queue contents */
void print_pq (pq_ptr);

/* Get the frame number from the paging queue */
int dequeue_NRU (pt_entry page_table[], pq_ptr *, pq_ptr *, int *);

/* Add a page/frame pair to the end of the paging queue */
void enqueue_NRU (pq_ptr *, pq_ptr *, int, int);

/* Get the frame number from the head of the paging queue */
int dequeue_FIFO (pt_entry page_table[], pq_ptr *, pq_ptr *, int *);

/* Add a page/frame pair to the end of the paging queue */
void enqueue_FIFO (pq_ptr *, pq_ptr *, int, int);

/* Get the frame number from the head of the paging queue */
int dequeue_SC(pt_entry page_table[], pq_ptr *, pq_ptr *, int *);

/* Add a page/frame pair to the end of the paging queue */
void enqueue_SC (pq_ptr *, pq_ptr *, int, int);

/* Get the frame number from the head of the paging queue */
int dequeue_ClockP(pt_entry[], pq_ptr *, pq_ptr *, int *);

/* Add a page/frame pair to the end of the paging queue */
void enqueue_ClockP(pq_ptr *, pq_ptr *, int, int);

/* Reset present, read, modified bits; check for paged-out & update po count */
void unmap_pagetable(pt_entry [], int, long *);

/* Remap the page/frame pair in the page table */
void map_pagetable (pt_entry [], int, int, int, long *, long *);

/* Remap the page/frame pair in the frame table */
void map_frametable (int frame_table[], int victim, int pagenum);

/* Calculate the total cost in cycles of all the operations performed. */
long long calc_SUM (int, long, long, long
		, long, long);

/* Print the summary statistics. */
void print_SUM(int, long, long, long, long, long, long long);

/* Assign the replacement algorithm for use with function pointers and naming */
pager_ptr parse_ptype(char);


