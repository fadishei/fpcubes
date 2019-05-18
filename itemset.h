#ifndef ITEMSET_H
#define ITEMSET_H

typedef struct
{
	int len;
	int *items;
} itemset_t;

typedef struct
{
	long len;
	int item_max;
	long lensum;
	itemset_t *itemsets;
} itemset_bag_t;

itemset_bag_t *itemset_bag_create(char *path);
void itemset_free(itemset_t *itemset);
void itemset_bag_free(itemset_bag_t *bag);
void itemset_bag_free_keep_stats(itemset_bag_t *bag);
void itemset_bag_free_keep_itemsets(itemset_bag_t *bag);

#endif