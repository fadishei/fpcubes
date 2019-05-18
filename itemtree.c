#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include "itemtree.h"
#include "bitset.h"

itemnode_t *itemtree_create(bitset_bag_t *bag, long minsup)
{
	int i;
	itemnode_t *node, *left;
	itemnode_t *hooker = (itemnode_t *)malloc(sizeof(itemnode_t));
	hooker->right = NULL;
	
	for (i=0; i<bag->len; i++)
	{
		if (bag->bitsets[i].card >= minsup)
		{
			itemnode_t *n = (itemnode_t *)malloc(sizeof(itemnode_t));
			n->item = i;
			n->bitset = bag->bitsets+i; // copying is expensive. point to the one in the bitset bag
			n->count = n->bitset->card;
			n->down = NULL;
			n->up = NULL;
			for (left=hooker, node=hooker->right; node!=NULL && node->item<n->item; left=node, node=node->right)
				;
			left->right = n;
			n->right = node;
		}		
		//else // keep it for bitset_bag_get_support
		//{
		//	bitset_free(bag->bitsets+i);
		//}
	}
	node = hooker->right;
	free(hooker);
	return node;
}

void itemtree_insert_down(itemnode_t *parent, itemnode_t *child)
{
	itemnode_t *node, *left;
	if (!parent->down)
	{
		parent->down=child;
		child->right=NULL;
	}
	else
	{
		for (left=parent->down, node=left->right; node!=NULL && node->item<child->item; left=node, node=node->right)
			;
		left->right=child;
		child->right=node;
	}
	child->up = parent;
}

void itemtree_print_rec(itemnode_t *node, int level)
{
	int i;
	itemnode_t *p;
	
	long lastmax = LONG_MAX;
	while (1)
	{
		// find next max
		long max = 0;
		for (p=node; p!=NULL; p=p->right)
			if (p->count>max && p->count<lastmax)
				max = p->count;
		if (max == 0)
			break;
		lastmax = max;
		
		// print all with max 
		for (p=node; p!=NULL; p=p->right)
		{
			if (p->count == max)
			{
				for (i=0; i<level; i++)
					printf(" ");
				printf("%d (%ld)\n", p->item, p->count);
				if (p->down)
					itemtree_print_rec(p->down, level+1);
			}
		}
	}
}

void itemtree_print(itemnode_t *root)
{
	if (root)
		itemtree_print_rec(root, 0);
}

int itemtree_len_rec(itemnode_t *node, int n)
{
	int d = (node->down)? itemtree_len_rec(node->down, n): 0;
	int r = (node->right)? itemtree_len_rec(node->right, n): 0;
	return n+d+r+1;
}

int itemtree_len(itemnode_t *root)
{
	return root? itemtree_len_rec(root, 0): 0;
}

int itemtree_count(itemnode_t *root)
{
	int n;
	itemnode_t *node;
	
	for (n=0, node=root; node; node=node->right)
		n += itemtree_count(node->down) + 1;
	
	return n;
}

int itemtree_count_maximal(itemnode_t *root)
{
	int n;
	itemnode_t *node;
	
	for (n=0, node=root; node; node=node->right)
		n += node->down? itemtree_count_maximal(node->down): 1;

	return n;

}

long itemtree_len_sum_rec(itemnode_t *root, int level)
{
	long n;
	itemnode_t *node;
	
	for (n=0, node=root; node; node=node->right)
		n += itemtree_len_sum_rec(node->down, level+1) + level;

	return n;
}

long itemtree_len_sum(itemnode_t *root)
{
	if (root)
		return itemtree_len_sum_rec(root, 1);
}

long itemtree_maximal_len_sum_rec(itemnode_t *root, int level)
{
	long n;
	itemnode_t *node;
	
	for (n=0, node=root; node; node=node->right)
		n += node->down? itemtree_maximal_len_sum_rec(node->down, level+1): level;

	return n;
}

long itemtree_maximal_len_sum(itemnode_t *root)
{
	if (root)
		return itemtree_maximal_len_sum_rec(root, 1);
}

void itemtree_free(itemnode_t *root)
{
	if (root)
	{
		if (root->down)
		{
			itemtree_free(root->down);
			root->down = NULL;
		}
		if (root->right)
		{
			itemtree_free(root->right);
			root->right=NULL;
		}
		if (root->up!=NULL && root->bitset!=NULL) // bitset of root nodes belongs to the initial bitset bag and should not be freed
		{
			bitset_free(root->bitset);
			root->bitset = NULL;
		}
		free(root);
	}
}

/*
void itemtree_free_keep_bitsets(itemnode_t *root)
{
	if (root)
	{
		if (root->down)
			itemtree_free_keep_bitsets(root->down);
		if (root->right)
			itemtree_free_keep_bitsets(root->right);
		free(root);
	}
}
*/
