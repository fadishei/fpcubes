#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include "util.h"
#include "cube.h"
#include "eclat.h"

#define QUERY_MAX_LEN	8191

void print_help(FILE *fp)
{
	fprintf(fp, "usage: fpc [options]\n");
	fprintf(fp, "options:\n");
	fprintf(fp, "-d <dir>      dataset dir. example folder tree path: day/1/hour/11/transactions.csv\n");
	fprintf(fp, "-h            print help\n");
	fprintf(fp, "-H            print header\n");
	fprintf(fp, "-i            idle seconds between queries to let cpu cool\n");
	fprintf(fp, "-m <sup>      minimum support. default 0.1\n");
	fprintf(fp, "-n            do not use cube aggregation\n");
	fprintf(fp, "-p            print frequent patterns\n");
	fprintf(fp, "-s            print stats\n");
}

int main(int argc, char *argv[])
{
	int i, c;
	char *datadir = NULL;
	double minsup = 0.1;
	int nocube=0, printhd = 0, printfp = 0, printst = 0, idle = 0;
	char query[QUERY_MAX_LEN];
	
	struct timespec start, end;
	while ((c=getopt(argc, argv, "d:hHi:m:nps")) != -1)
	{
		switch (c)
		{
			case 'd':
				datadir = optarg;
				break;
			case 'h':
				print_help(stdout);
				exit(0);
			case 'H':
				printhd = 1;
				break;
			case 'i':
				idle = atoi(optarg);
				break;
			case 'm':
				minsup = atof(optarg);
				if (minsup<=0 || minsup>1)
				{
					fprintf(stderr, "invalid minsup %s\n", optarg);
					exit(1);
				}
				break;
			case 'n':
				nocube = 1;
				break;
			case 'p':
				printfp = 1;
				break;
			case 's':
				printst = 1;
				break;
			default:
				print_help(stderr);
				exit(1);
		}
	}
	if (!(printhd || printfp&&datadir || printst&&datadir))
	{
		print_help(stderr);
		exit(1);
	}

	DEBUG("creating query tree\n");
	cubenode_t *qtree = query_tree_create(datadir, nocube);
	//DEBUG("===== query tree: =====\n");
	//cubetree_print(qtree, 0);
	//DEBUG("=========================\n");
	if (!nocube)
	{
		cnt_bitmap_and=0;
		DEBUG("pre-mining query tree\n");
		clock_gettime(CLOCK_REALTIME, &start);
		query_tree_premine(qtree, minsup);
		clock_gettime(CLOCK_REALTIME, &end);
		printf("# premine: t=%f,m=%lu,ands=%lu\n", (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec)/1000000000.0, stat_get_vsize(), cnt_bitmap_and);
		//DEBUG("===== query tree: =====\n");
		//cubetree_print(qtree, 0);
		//DEBUG("=========================\n");
	}
	DEBUG("waiting for query\n");

	if (printhd)
		printf("query,cuboids,time,memory,fpcnt,fplenavg,ands,trancnt,tranlenavg%s\n", nocube? "": ",digs,leafdigs");
	
	i = 0;
	while (fgets(query, QUERY_MAX_LEN, stdin)) 
	{
		double t;
		int cuboids, fpcount, fplensum;
		long m, trancount, tranlensum;
		cnt_bitmap_and = 0;
		
		DEBUG("creating partition list\n");
		cubenode_t *plist = partition_list_create(qtree, query);
		cuboids = partition_list_len(plist);
		trancount = partition_list_tran_count(plist);
		tranlensum = partition_list_tran_len_sum(plist);
		//DEBUG("==== partition list: ====\n");
		//partition_list_print(plist);
		//DEBUG("=========================\n");
		
		if (nocube)
		{
			DEBUG("creating itemset bag from partition list\n");
			itemset_bag_t *ibag = partition_list_aggr(plist);
			clock_gettime(CLOCK_REALTIME, &start);
			DEBUG("creating bitset bag\n");
			bitset_bag_t *bbag = bitset_bag_create(ibag);
			DEBUG("creating itemset tree\n");
			itemnode_t *tree = itemtree_create(bbag, SUP(minsup, ibag->len));
			DEBUG("mining itemset tree\n");
			eclat(tree, SUP(minsup, ibag->len));
			clock_gettime(CLOCK_REALTIME, &end);
			t = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec)/1000000000.0;
			m = stat_get_vsize();
			fpcount = itemtree_count(tree);
			fplensum = itemtree_len_sum(tree);
			if (printfp)
			{
				DEBUG("===== itemset tree: =====\n");
				itemtree_print(tree);
				DEBUG("=========================\n");
			}
			DEBUG("freeing itemset tree\n");
			//itemtree_free_keep_bitsets(tree);
			itemtree_free(tree);
			DEBUG("freeing bitset bag\n");
			bitset_bag_free(bbag);
			DEBUG("freeing itemset bag\n");
			itemset_bag_free_keep_itemsets(ibag);
			
		}
		else
		{
			digs_cnt=0;
			leafdigs_cnt=0;
			clock_gettime(CLOCK_REALTIME, &start);
			DEBUG("creating collect tree\n");
			cubenode_t *ctree = collect_tree_create(plist, 2);
			//DEBUG("===== collect tree: =====\n");
			//cubetree_print(ctree, 0);
			//DEBUG("=========================\n");
			DEBUG("mining collect tree\n");
			get_frequent(ctree);
			//DEBUG("===== collect tree: =====\n");
			//cubetree_print(ctree, 0);
			//DEBUG("=========================\n");
			clock_gettime(CLOCK_REALTIME, &end);
			t = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec)/1000000000.0;
			m = stat_get_vsize();
			fpcount = itemtree_count(ctree->tree);
			fplensum = itemtree_len_sum(ctree->tree);
			if (printfp)
			{
				DEBUG("===== itemset tree: =====\n");
				itemtree_print(ctree->tree);
				DEBUG("=========================\n");
			}
			//collect_tree_free_keep_bitsets(ctree);
			collect_tree_free(ctree);
		}
		
		partition_list_free(plist);
		
		if (printst)
		{
			printf("%d,%d,%f,%lu,%u,%f,%lu,%lu,%f", i, cuboids, t, m, fpcount, (double)fplensum/(double)fpcount, cnt_bitmap_and, trancount, (double)tranlensum/(double)trancount);
			if (!nocube)
				printf(",%lu,%lu", digs_cnt, leafdigs_cnt);
			printf("\n");
			fflush(stdout);
		}
		sleep(idle);
		DEBUG("waiting for query\n");
		i++;
	}
	
	query_tree_free(qtree);
}
