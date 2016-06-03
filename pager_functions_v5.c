#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "mmu_defines.h"
#define RANDS 100000
#define TABLE 64

extern pager_type NRU, LRU, Random, FIFO, SC, ClockP, ClockV, AgingP, AgingV;
extern int randvals[];		// randvals[0] is count of random numbers in file
extern int frames;
extern int op_count;
int ofs = 1;

/* Helper function to return a randomized value. */
int myrandom(int);

void getrands(FILE *rand_file, int randvals[]) {
	int f = 0;
	fscanf(rand_file, "%d", &randvals[f]);
	int count = randvals[f];

	for (f = 1; f<=count; f++) {
		fscanf(rand_file, "%d", &randvals[f]);
	}

	fclose (rand_file);
}

int file_readline(FILE *rw_page, int *rw, int *pagenum, int *op_count) {
	int n = 0;

	while (n == 0) {		
		n = fscanf(rw_page, "%d%d", rw, pagenum);
		if (n == 0) {
			printf("Skipping #s...\n");
			fscanf(rw_page, "%*[^\n]\n", NULL);
		}	
	}
	if (n != -1) (*op_count)++;
	// n == -1 when EOF is reached
	return n;			
}

void initialize_frametable(int frame_table[]) {
	int i;
	for (i = 0; i < TABLE; i++) frame_table[i] = -1;
}

void print_frametable(int frame_table[], int frames) {
	int i;
	for (i = 0; i < frames; i++ ) {
		if (frame_table[i] > -1) printf("%d ", frame_table[i]);
		else printf("* ");
	}
	printf("\n");
}

void fill_tables(pt_entry page_table[], int frame_table[], int rw, int pagenum, 
	int *frame_count, long *m_count, long *z_count, long *pi_count) {
	if (page_table[pagenum].p == 0) {
		page_table[pagenum].frame = *frame_count;
		page_table[pagenum].p = 1;
		page_table[pagenum].r = 1;
		frame_table[*frame_count] = pagenum;
		(*frame_count)++;
		(*m_count)++;
		(*z_count)++;
	}
	if (rw == 1) page_table[pagenum].m = 1;
}

void print_pagetable(pt_entry page_table[]) {
	int i;
	for (i = 0; i < TABLE; i++) {	
		if (page_table[i].p == 1) {
			printf("%d:", i);
			if (page_table[i].r == 1) printf("R");
				else printf("-");
			if (page_table[i].m == 1) printf("M");
				else printf("-");
			if (page_table[i].o == 1) printf("S");
				else printf("-");
		} else {
			if (page_table[i].o == 1) printf("#");
			else printf("*");
		}
		printf(" ");
	}
	printf("\n");
}

void print_output(pt_entry page_table[], int old_page, int frame, int rw, int pagenum, int op_count) {
	printf("==> inst: %d %d\n", rw, pagenum);
	if (old_page != pagenum) {
		if (old_page > -1) {
			printf("%d: UNMAP%4d%4d\n", op_count - 1, old_page, frame);
			if (page_table[old_page].m == 1) printf("%d: OUT%6d%4d\n", op_count - 1, old_page, frame);
			if (page_table[pagenum].o == 1) printf("%d: IN%7d%4d\n", op_count - 1, pagenum, frame);
		}
		if (page_table[pagenum].o == 0) printf("%d: ZERO%9d\n", op_count - 1, frame);
		printf("%d: MAP%6d%4d\n", op_count - 1, pagenum, frame);
	}
}

int is_empty(pq_ptr q_head) {
	return q_head == NULL;
}

void print_pq(pq_ptr q_head) {
	if (q_head == NULL) printf("Queue is empty.\n");
	else {
		while (q_head != NULL) {
			printf("%d-->%d ", q_head->page_num, q_head->frame_num);
			q_head = q_head->next_ptr;
		}
		printf("NULL\n");
	}
}

int dequeue_NRU(pt_entry page_table[], pq_ptr *q_head, pq_ptr *q_tail, int *old_page) {
	int victim;
	static int rep_count;
	pq_ptr tmp_ptr = NULL, found_ptr = NULL;
	int refclass[frames];
	int i, found = 0;

	rep_count++;
	if (rep_count % 10 == 0) {
		for (i = 0; i < TABLE; i++) {
			if (page_table[i].p == 1) page_table[i].r = 0;
		}
	}

	for (i = 0; i < frames; i++) {
		refclass[i]=0;
	}
	
	for (i = 0; i < TABLE; i++) {
		if (page_table[i].p == 1 && page_table[i].r == 0 && page_table[i].m == 0) {
			refclass[found] = page_table[i].frame;
			found++;
		}
	}
	if (found == 0) {
		for (i = 0; i < TABLE; i++) {
			if (page_table[i].p == 1 && page_table[i].r == 0 && page_table[i].m == 1) {
				refclass[found] = page_table[i].frame;
				found++;
			}
		}
	}
	if (found == 0) {
		for (i = 0; i < TABLE; i++) {
			if (page_table[i].p == 1 && page_table[i].r == 1 && page_table[i].m == 0) {
				refclass[found] = page_table[i].frame;
				found++;
			}
		}
	}
	if (found == 0) {
		for (i = 0; i < TABLE; i++) {
			if (page_table[i].p == 1 && page_table[i].r == 1 && page_table[i].m == 1) {
				refclass[found] = page_table[i].frame;
				found++;
			}
		}
	}

	printf("\nfound = %d\n", found);
	victim = refclass[myrandom(found)];
	printf("randomly selected frame is %d\n", victim);
	printf("the queue is: ");
	print_pq(*q_head);
	tmp_ptr = *q_head;
	if (tmp_ptr->frame_num == victim) {
		*old_page = tmp_ptr->page_num;
		*q_head = (*q_head)->next_ptr;
	} else {
		while (tmp_ptr->next_ptr->frame_num != victim) {
			tmp_ptr = tmp_ptr->next_ptr;
		}
		*old_page = tmp_ptr->next_ptr->page_num;
		found_ptr = tmp_ptr->next_ptr;						// found_ptr is the node to remove
		tmp_ptr->next_ptr = tmp_ptr->next_ptr->next_ptr;	// will be assigned NULL if found node is last
		if (tmp_ptr->next_ptr == NULL) *q_tail = tmp_ptr;
		tmp_ptr = found_ptr;
	}
	if (*q_head == NULL) *q_tail = NULL;
	free (tmp_ptr);
	return victim;
}

void enqueue_NRU(pq_ptr *q_head, pq_ptr *q_tail, int frame, int pagenum) {
	pq_ptr new_node;
	new_node = malloc(sizeof(p_queue));

	if (new_node != NULL) {
		new_node->page_num = pagenum;
		new_node->frame_num = frame;
		new_node->next_ptr = NULL;

		if (is_empty(*q_head)) *q_head = new_node;
		else (*q_tail)->next_ptr = new_node;
		*q_tail = new_node;
	} else {
		printf("%d not queued: not enough memory.\n", pagenum);
	}
}

int dequeue_FIFO(pt_entry page_table[], pq_ptr *q_head, pq_ptr *q_tail, int *old_page) {
	int victim;
	pq_ptr tmp_ptr;

	victim = (*q_head)->frame_num;
	*old_page = (*q_head)->page_num;
	tmp_ptr = *q_head;
	*q_head = (*q_head)->next_ptr;
	if (*q_head == NULL) *q_tail = NULL;
	free (tmp_ptr);
	return victim;
}

void enqueue_FIFO(pq_ptr *q_head, pq_ptr *q_tail, int frame, int pagenum) {
	pq_ptr new_node;
	new_node = malloc(sizeof(p_queue));

	if (new_node != NULL) {
		new_node->page_num = pagenum;
		new_node->frame_num = frame;
		new_node->next_ptr = NULL;

		if (is_empty(*q_head)) *q_head = new_node;
		else (*q_tail)->next_ptr = new_node;
		*q_tail = new_node;
	} else {
		printf("%d not queued: not enough memory.\n", pagenum);
	}
}

int dequeue_SC(pt_entry page_table[], pq_ptr *q_head, pq_ptr *q_tail, int *old_page) {
	int victim;
	pq_ptr tmp_ptr;

	while (page_table[(*q_head)->page_num].r == 1){
		page_table[(*q_head)->page_num].r = 0;
		tmp_ptr = *q_head;
		*q_head = (*q_head)->next_ptr;
		(*q_tail)->next_ptr = tmp_ptr;
		*q_tail = (*q_tail)->next_ptr;
	}
	victim = (*q_head)->frame_num;
	*old_page = (*q_head)->page_num;
	tmp_ptr = *q_head;
	*q_head = (*q_head)->next_ptr;

	if (*q_head == NULL) *q_tail = NULL;
	free (tmp_ptr);
	return victim;
}

void enqueue_SC(pq_ptr *q_head, pq_ptr *q_tail, int frame, int pagenum) {
	pq_ptr new_node;
	new_node = malloc(sizeof(p_queue));

	if (new_node != NULL) {
		new_node->page_num = pagenum;
		new_node->frame_num = frame;
		new_node->next_ptr = NULL;

		if (is_empty(*q_head)) *q_head = new_node;
		else (*q_tail)->next_ptr = new_node;
		*q_tail = new_node;
	} else {
		printf("%d not queued: not enough memory.\n", pagenum);
	}
}

int dequeue_ClockP(pt_entry page_table[], pq_ptr *q_head, pq_ptr *q_tail, int *old_page) {
	int victim;
	pq_ptr tmp_ptr;

	while (page_table[(*q_head)->page_num].r == 1){
		page_table[(*q_head)->page_num].r = 0;
		tmp_ptr = *q_head;
		*q_head = (*q_head)->next_ptr;
		(*q_tail)->next_ptr = tmp_ptr;
		*q_tail = (*q_tail)->next_ptr;
	}
	victim = (*q_head)->frame_num;
	*old_page = (*q_head)->page_num;
	tmp_ptr = *q_head;
	*q_head = (*q_head)->next_ptr;

	if (*q_head == NULL) *q_tail = NULL;
	free (tmp_ptr);
	return victim;
}

void enqueue_ClockP(pq_ptr *q_head, pq_ptr *q_tail, int frame, int pagenum) {
	pq_ptr new_node;
	new_node = malloc(sizeof(p_queue));

	if (new_node != NULL) {
		new_node->page_num = pagenum;
		new_node->frame_num = frame;
		new_node->next_ptr = *q_head;

		if (is_empty(*q_head)) *q_head = new_node;
		else (*q_tail)->next_ptr = new_node;
		*q_tail = new_node;
	} else {
		printf("%d not queued: not enough memory.\n", pagenum);
	}
}

void unmap_pagetable(pt_entry page_table[], int old_page, long *po_count) {
	page_table[old_page].p = 0;
	if (page_table[old_page].m == 1) {
		(*po_count)++;
		if (page_table[old_page].o == 0) page_table[old_page].o = 1;
	}
	page_table[old_page].r = 0;
	page_table[old_page].m = 0;
}

void map_pagetable(pt_entry page_table[], int victim, int pagenum, int rw
		, long *pi_count, long *z_count) {
	page_table[pagenum].frame = victim;
	page_table[pagenum].p = 1;
	if (rw == 1) page_table[pagenum].m = 1;
	page_table[pagenum].r = 1;
	if (page_table[pagenum].o == 1) (*pi_count)++;
	else (*z_count)++;
}

void map_frametable(int frame_table[], int victim, int pagenum) {
	frame_table[victim] = pagenum;
}

long long calc_SUM (int op_count, long um_count, long m_count, long pi_count
		, long po_count, long z_count){
	long long totalcost = (400 * (um_count + m_count)) + (150 * z_count) 
		+ (3000 * (pi_count + po_count)) + (long)op_count;
	return totalcost; 
}

void print_SUM(int op_count, long um_count, long m_count, long pi_count
		, long po_count, long z_count, long long totalcost) {
	printf("SUM %d U=%ld M=%ld I=%ld O=%ld Z=%ld ===> %llu\n", op_count
		, um_count, m_count, pi_count, po_count, z_count, totalcost);
}

pager_ptr parse_ptype(char pager) {
	pager_ptr p_type;

	switch (pager) {

		case 'N':
		p_type = &NRU;
		break;

		case 'l':
		p_type =  &LRU;
		break;

		case 'r':
		p_type =  &Random;
		break;

		case 'f':
		p_type =  &FIFO;
		break;

		case 's':
		p_type =  &SC;
		break;

		case 'c':
		p_type =  &ClockP;
		break;

		case 'X':
		p_type =  &ClockV;
		break;

		case 'a':
		p_type =  &AgingP;
		break;

		case 'Y':
		p_type =  &AgingV;
		break;
	}

	return p_type;
}

int myrandom(int found) {	
	int randtemp;
	if (ofs > randvals[0]) {
		ofs = 1;
	}
	printf("offset = %d, random value = %d\n", ofs, randvals[ofs]);
	randtemp = (randvals[ofs] % found);
	printf("randomly selected index is %d\n", randtemp);
	ofs++;
	return randtemp;
}


