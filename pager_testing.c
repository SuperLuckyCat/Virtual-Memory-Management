#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "mmu_defines.h"
#define RANDS 100000
#define TABLE 64

pager_type NRU, LRU, Random, FIFO, SC, ClockP, ClockV, AgingP, AgingV;
int randvals[RANDS] = {0};		// randvals[0] is count of random numbers in file
int frames = 32;
int op_count = 0;

int main(int argc, char *argv[]) {
	FILE *rw_page;
	FILE *rand_file;
	char pager = 'l';
	char *print_opts = NULL;
	int c;
	int rw = 0, pagenum = 0, frame_count = 0, victim = -1, old_page = -1;
	long um_count = 0, m_count = 0, pi_count = 0, po_count = 0, z_count = 0;
	long long totalcost = 0;
	pt_entry page_table[TABLE] = {{0}};
	int frame_table[TABLE];
	pq_ptr q_head = NULL, q_tail = NULL;
	pager_ptr p_type;
	NRU.p_name = "FIFO";
	NRU.add_page = enqueue_NRU;
	NRU.get_victim = dequeue_NRU;
	FIFO.p_name = "FIFO";
	FIFO.add_page = enqueue_FIFO;
	FIFO.get_victim = dequeue_FIFO;
	SC.p_name = "SC";
	SC.add_page = enqueue_SC;
	SC.get_victim = dequeue_SC;
	ClockP.p_name = "ClockP";
	ClockP.add_page = enqueue_ClockP;
	ClockP.get_victim = dequeue_ClockP;

	while ((c = getopt (argc, argv, "a:o:f:")) != -1) {
		switch (c) {

			case 'a': 
			pager = *optarg;
			break;

			case 'o': 
			print_opts = optarg;
			break;

			case 'f':
			frames = atoi(optarg);

			case '?':
			if (optopt == 'a')
				fprintf (stderr, "Option -%c requires an argument.\n", optopt);
			if (optopt == 'o')
				fprintf (stderr, "Option -%c requires an argument.\n", optopt);

			default:
			break;
		}
	}

	printf("pager type is %c, print options are %s, and frame count is %d\n", pager, print_opts, frames);

	p_type = parse_ptype(pager);

	if ((rand_file = fopen(argv[(argc - 1)], "r")) == NULL) {
		printf ("Sorry, can't open the random number file!\n");
	} else {
		getrands(rand_file, randvals);
		printf("The number of random values is %d\n", randvals[0]);
	}
	if ((rw_page = fopen(argv[(argc - 2)], "r")) == NULL) {
		printf ("Sorry, can't open the input file!\n");
	} else {
		initialize_frametable(frame_table);
		while ((frame_table[frames - 1] == -1) && 
			(file_readline(rw_page, &rw, &pagenum, &op_count) > -1)) {
			p_type->add_page(&q_head, &q_tail, frame_count, pagenum);
			print_output(page_table, old_page, frame_count, rw, pagenum, op_count);
			fill_tables(page_table, frame_table, rw, pagenum, &frame_count, &m_count, &z_count, &pi_count);
			print_pagetable(page_table);
			print_frametable(frame_table, frames);

		}
		while (file_readline(rw_page, &rw, &pagenum, &op_count) > -1) {
			if (page_table[pagenum].p == 0) {
				victim = p_type->get_victim(page_table, &q_head, &q_tail, &old_page);
				p_type->add_page(&q_head, &q_tail, victim, pagenum);
			} else if (rw == 1 && page_table[pagenum].m == 0) {
				page_table[pagenum].m = 1;
				page_table[pagenum].r = 1;
				old_page = pagenum;
				victim = -1;
			} else if (rw == 0 && page_table[pagenum].r == 0){
				page_table[pagenum].r = 1;
				old_page = pagenum;
				victim = -1;
			} else {
				old_page = pagenum;
				victim = -1;
			}
			print_output(page_table, old_page, victim, rw, pagenum, op_count);
			if (page_table[pagenum].p == 0) {
				um_count++;
				m_count++;
				unmap_pagetable(page_table, old_page, &po_count);
				map_pagetable(page_table, victim, pagenum, rw, &pi_count, &z_count);
				map_frametable(frame_table, victim, pagenum);
			}
			print_pagetable(page_table);
			print_frametable(frame_table, frames);
			// print_pq(q_head);
		}

		fclose(rw_page);

		totalcost = (long long)(calc_SUM (op_count, um_count, m_count, pi_count
		, po_count, z_count));
		print_SUM(op_count, um_count, m_count, pi_count
			, po_count, z_count, totalcost);
		
	}
}
