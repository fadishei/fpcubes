#include <stdlib.h>
#include "bitset.h"

bitset_bag_t *bitset_bag_create(itemset_bag_t *ibag)
{
	long i, j;
	
	bitset_bag_t *bbag;
	
	bbag = (bitset_bag_t *)malloc(sizeof(bitset_bag_t));
	if (!bbag)
		goto e1;
	
	bbag->len = 0;
	bbag->bitsets = (bitset_t *)malloc((ibag->item_max+1)*sizeof(bitset_t));
	if (!bbag->bitsets)
		goto e2;
	for (i=0; i<ibag->item_max+1; i++)
	{
		bbag->bitsets[i].bitmap = wrapped_bitmap_create();
		bbag->bitsets[i].card = 0;
		if (!bbag->bitsets[i].bitmap)
			goto e2;
		bbag->len++;
	}
	for (i=0; i<ibag->len; i++)
		for (j=0; j<ibag->itemsets[i].len; j++)
		{
			wrapped_bitmap_add(bbag->bitsets[ibag->itemsets[i].items[j]].bitmap, i);
			bbag->bitsets[ibag->itemsets[i].items[j]].card++;
		}
	return bbag;
	
e2:
	bitset_bag_free(bbag);
e1:
	return NULL;
}

void bitset_free(bitset_t *set)
{
	if (set!=NULL)
	{
		wrapped_bitmap_free(set->bitmap);
		set->bitmap = NULL;
	}
}

void bitset_bag_free(bitset_bag_t *b)
{
	int i;
	if (b->bitsets)
	{
		for (i=0; i<b->len; i++)
			bitset_free(b->bitsets+i);
		free(b->bitsets);
	}
	free(b);
}

void bitset_bag_free_keep_bitsets(bitset_bag_t *b)
{
	// we won't free bitsets[] since some of it is already used by itemtree and the others are already freed when creating tree
	free(b);
}

long bitset_bag_get_support(itemset_t *itemset, bitset_bag_t *bag)
{
	wrapped_bitmap_t *r1, *r2;
	long i;

	if (itemset->len == 0)
		return 0;
	if (itemset->items[0]>=bag->len || bag->bitsets[itemset->items[0]].bitmap==NULL)  // different partitions may have different max-item and thus different array sizes. moreover, a bag may lack some items. moreover part is excessive
		return 0;
	if (itemset->len == 1)
		return bag->bitsets[itemset->items[0]].card;
	if (itemset->items[1]>=bag->len || bag->bitsets[itemset->items[1]].bitmap==NULL)
		return 0;
	
	r1 = wrapped_bitmap_and(bag->bitsets[itemset->items[0]].bitmap, bag->bitsets[itemset->items[1]].bitmap);
	for (i=2; i<itemset->len; i++)
	{
		if (itemset->items[i]>=bag->len || bag->bitsets[itemset->items[i]].bitmap==NULL)
		{
			wrapped_bitmap_free(r1);
			return 0;
		}
		r2 = wrapped_bitmap_and(r1, bag->bitsets[itemset->items[i]].bitmap);
		wrapped_bitmap_free(r1);
		r1 = r2;
	}
	i = wrapped_bitmap_get_cardinality(r1);
	wrapped_bitmap_free(r1);
	return i;
}
