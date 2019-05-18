#ifndef CUBE_H
#define CUBE_H

#include "itemset.h"
#include "bitset.h"
#include "itemtree.h"

extern long digs_cnt, leafdigs_cnt;

typedef struct cubenode
{								// applicabe to (dist/grow/collect)
	char *name;					// dg 
	itemnode_t *tree; 			//  gc
	bitset_bag_t *bitsets; 		//  g
	long items;					//  g
	uint8_t visited;			//  g
	double minsup;				//  gc
	struct cubenode *right; 	//  gc
	struct cubenode **children; // d c
	int len;					// d c
	itemset_bag_t *itemsets;	// exessive. only for easy comparison with eclat
} cubenode_t;

cubenode_t *query_tree_create(char *dir, int keep_itemsets);
void query_tree_premine(cubenode_t *root, double minsup);
void query_tree_free(cubenode_t *root);
cubenode_t *partition_list_create(cubenode_t *qtree, char *query);
void partition_list_print(cubenode_t *head);
void partition_list_free(cubenode_t *head);
int partition_list_len(cubenode_t *head);
long partition_list_tran_count(cubenode_t *head);
long partition_list_tran_len_sum(cubenode_t *head);
itemset_bag_t *partition_list_aggr(cubenode_t *head);
cubenode_t *collect_tree_create(cubenode_t *plist, int degree);
//void collect_tree_free_keep_bitsets(cubenode_t *root);
void collect_tree_free(cubenode_t *root);
itemnode_t *get_frequent(cubenode_t *root);
void cubenode_print(cubenode_t *root);
void cubetree_print(cubenode_t *root, int pad);

#endif