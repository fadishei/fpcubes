#include <dirent.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "cube.h"
#include "util.h"
#include "lbound.h"
#include "eclat.h"

// stat counters. global. not thread safe
long digs_cnt=0, leafdigs_cnt=0;

cubenode_t *query_tree_create(char *dir, int keep_itemsets)
{
	struct dirent *ent;
	char p[PATH_MAX+1];
	char *e;
	strcpy(p, dir);

	DIR *d = opendir(p);
	if (!d)
		return NULL;
	
	DEBUG("processing folder %s\n", dir);
	cubenode_t *parent = NULL;
	while ((ent = readdir(d)) != NULL) 
	{			
		if (ent->d_type!=DT_DIR && strcmp(ent->d_name, "transactions.csv")==0)
		{
			parent = (cubenode_t *)malloc(sizeof(cubenode_t));
			parent->name = strdup(ent->d_name);
			strcat(p, "/");
			strcat(p, ent->d_name);
			itemset_bag_t *ibag = itemset_bag_create(p);
			bitset_bag_t *bbag = bitset_bag_create(ibag);
			parent->items = ibag->len;
			parent->bitsets = bbag;
			parent->tree = NULL;
			parent->visited = 0;
			parent->name = strdup(p);
			parent->right = NULL;
			parent->children = NULL;
			parent->len = 0;
			if (keep_itemsets)
				parent->itemsets = ibag;    // used for eclat comparison
			else
			{
				itemset_bag_free_keep_stats(ibag);
				parent->itemsets = ibag;
			}
			closedir (d);
			return parent;
		}
		else if (ent->d_type==DT_DIR && strcmp(ent->d_name, ".")!=0 && strcmp(ent->d_name, "..")!=0)
		{
			parent = (cubenode_t *)malloc(sizeof(cubenode_t));
			parent->name = strdup(ent->d_name);
			parent->children = NULL;
			parent->tree = NULL;
			parent->bitsets = NULL;
			parent->itemsets = NULL;
			parent->visited = 0;
			closedir (d);
			break;
		}
	}
	if (!parent)
	{
		fprintf(stderr, "error: invalid dir structure");
		return NULL;
	}

	strcat(p, "/");
	strcat(p, parent->name);
	d = opendir(p);
	if (!d) 
		return NULL;
	int i=0, max=0;
	while ((ent = readdir(d)) != NULL) 
	{
		if (ent->d_type!=DT_DIR || strcmp(ent->d_name, ".")==0 || strcmp(ent->d_name, "..")==0)
			continue;
		i++;
		int n = (int)strtol(ent->d_name, &e, 10);
		if (*e != '\0')
		{
			fprintf(stderr, "invalid folder name");
			return NULL;
		}
		if (n>max)
			max = n;
	}
	closedir(d);
	parent->children = (cubenode_t **)malloc((max+1)*sizeof(cubenode_t *));
	parent->len = max+1;
	d = opendir(p);
	if (!d) 
		return NULL;
	while ((ent = readdir(d)) != NULL) 
	{
		if (ent->d_type!=DT_DIR || strcmp(ent->d_name, ".")==0 || strcmp(ent->d_name, "..")==0)
			continue;
		int n = (int)strtol(ent->d_name, &e, 10);
		if (*e != '\0')
		{
			fprintf(stderr, "invalid folder name");
			return NULL;
		}
		
		char p2[PATH_MAX];
		sprintf(p2, "%s/%d", p, n);
		parent->children[n] = query_tree_create(p2, keep_itemsets);
	}
	closedir(d);
	return parent;
}

void query_tree_premine(cubenode_t *root, double minsup)
{
	if (root->children == NULL)
	{
		root->minsup = minsup;
		long s = SUP(root->minsup, root->items);
		DEBUG("==== %s %f %lu %lu ====\n", root->name, root->minsup, root->items, s);
		root->tree = itemtree_create(root->bitsets, s);
		eclat(root->tree, s);
		//itemtree_print(root->tree);
	}
	else
	{
		int i;
		for (i=0; i<root->len; i++)
			query_tree_premine(root->children[i], minsup);
	}
}

void query_tree_free(cubenode_t *root)
{
	if (root->children == NULL)
	{
		itemtree_free(root->tree);
		if (root->itemsets)
			itemset_bag_free(root->itemsets);
		bitset_bag_free_keep_bitsets(root->bitsets);
	}
	else
	{
		int i;
		for (i=0; i<root->len; i++)
			query_tree_free(root->children[i]);
	}	
}

char *partition_list_create_rec(cubenode_t *root, char *query, cubenode_t *head)
{
	uint8_t isrange, digitseen;
	int nprev, n;
	char *p;
	int i;
	char *ret = NULL;
	
	if (!root->children)
	{
		//printf("OK %s -> %s\n", root->name, root->leaf->name);
		cubenode_t *tmp=head->right;
		if (!root->visited) // FIXME: shared. not thread-safe. needs mutex
		{
			root->visited = 1;
			head->right = root;
			root->right = tmp;
		}
		ret = query;
		
	}
	else
	{
		int l = strlen(root->name);
		if (strncmp(root->name, query+1, l)!=0)
			printf("invalid query 1\n");
		
		p = query+l+2;
		
		char *p2 = p;
		while (*p2!='\0' && *p2!='/')
			p2++;
		
		n = 0;
		nprev = n;
		isrange = 0;
		digitseen = 0;
		while(1)
		{
			if (*p>='0' && *p<='9')
			{
				n = n*10+(*p)-'0';
				digitseen = 1;
			}
			else if (*p=='-')
			{
				if (!digitseen)
				{
					fprintf(stderr, "invalid query 2\n");
					return NULL;
				}
				nprev = n;
				n = 0;
				isrange = 1;
				digitseen = 0;
			}
			else if (*p==',' || *p=='/' || *p=='\0')
			{
				if (!digitseen)
				{
					fprintf(stderr, "invalid query 3 q:%s p:%s\n", query, p);
					return NULL;
				}
				else if (isrange)
				{
					for (i=nprev; i<=n; i++)
					{
						if (i>=root->len)
						{
							fprintf(stderr, "invalid query 5\n");
							return NULL;
						}
						char *s = partition_list_create_rec(root->children[i], p2, head);
						if (ret==NULL)
							ret = s;
						else if (ret!=s)
						{
							fprintf(stderr, "invalid query 9");
							return NULL;
						}
					}
					nprev = n;
					n = 0;
					isrange = 0;
					digitseen = 0;
				}
				else
				{
					if (n>=root->len)
					{
						fprintf(stderr, "invalid query 4\n");
						return NULL;
					}
					char *s = partition_list_create_rec(root->children[n], p2, head);
					if (ret==NULL)
						ret = s;
					else if (ret!=s)
					{
						fprintf(stderr, "invalid query 9");
						return NULL;
					}
					nprev = n;
					n = 0;
					digitseen = 0;
				}
			}
			if (*p=='\0' || *p=='/')
				break;
			p++;
		}
		return ret;
	}
}

cubenode_t *partition_list_create(cubenode_t *qtree, char *query)
{
	cubenode_t *hooker = (cubenode_t *)malloc(sizeof(cubenode_t));
	hooker->right = NULL;
	while (*query!='\0')
		query = partition_list_create_rec(qtree, query, hooker);
	cubenode_t *head = hooker->right;
	free(hooker);
	
	return head;
}

void partition_list_print(cubenode_t *head)
{
	cubenode_t *p;
	for (p=head; p!=NULL; p=p->right)
		cubenode_print(p);
}

int partition_list_len(cubenode_t *head)
{
	int i=0;
	cubenode_t *p;
	for (p=head; p!=NULL; p=p->right)
		i++;
	return i;
}

long partition_list_tran_count(cubenode_t *head)
{
	long n=0;
	cubenode_t *p;
	for (p=head; p!=NULL; p=p->right)
		n += p->itemsets->len;
	return n;
}

long partition_list_tran_len_sum(cubenode_t *head)
{
	long n=0;
	cubenode_t *p;
	for (p=head; p!=NULL; p=p->right)
		n += p->itemsets->lensum;
	return n;
}

itemset_bag_t *partition_list_aggr(cubenode_t *head)
{
	cubenode_t *p;
	int i, j, n, max=0;
	
	for(n=0, p=head; p!=NULL; p=p->right)
	{
		n += p->itemsets->len;
		if (p->itemsets->item_max>max)
			max = p->itemsets->item_max;
	}
	
	itemset_bag_t *bag = (itemset_bag_t *)malloc(sizeof(itemset_bag_t));
	bag->len = n;
	bag->item_max = max;
	bag->itemsets = (itemset_t *)malloc(sizeof(itemset_t)*n);
	
	for (i=0, p=head; p!=NULL && i<n; p=p->right)
	{
		for (j=0; j<p->itemsets->len && i<n; j++, i++)
			bag->itemsets[i] = p->itemsets->itemsets[j];
	}
	bag->len = i;
	return bag;
}

void partition_list_free(cubenode_t *head)
{
	cubenode_t *p1=head, *p2=head->right;
	
	while (p1!=NULL)
	{
		p2 = p1->right;
		p1->right = NULL;
		p1->visited = 0;
		p1 = p2;
	}
}

cubenode_t *collect_tree_create(cubenode_t *plist, int degree)
{
	cubenode_t *node, *parent, *head;
	node = plist;
	head = NULL;
	parent = NULL;
	int c = 'A';
	while(1)
	{
		if (parent == NULL)
		{
			parent = (cubenode_t *)malloc(sizeof(cubenode_t));
			parent->children = (cubenode_t **)malloc(degree*sizeof(cubenode_t*));
			parent->len = 0;
			parent->items = 0;
			parent->tree = NULL;
			parent->right = NULL;
			parent->name = strdup(" ");
			parent->name[0] = c++;
			parent->minsup = node->minsup;
		}
		while (parent->len < degree && node)
		{
			parent->children[parent->len++] = node;
			parent->items += node->items;
			node = node->right;
		}
		if (parent->len == degree)
		{
			parent->right = head;
			head = parent;
			parent = NULL;
		}
		if (node == NULL)
		{
			node = head;
			head = NULL;
		}
		if (node==NULL && head==NULL && parent!=NULL && parent->right==NULL)
			return parent;
		if (head==NULL && parent==NULL && node!=NULL && node->right==NULL)
			return node;
	}
}

/*
void collect_tree_free_keep_bitsets(cubenode_t *root)
{
	int i;

	//printf("root=%s children=%s len=%d, tree=%s\n", root? "Y": "N", root->children? "Y": "N", root->len, root->tree? "Y": "N");
	if (root->children)
	{
		for (i=0; i<root->len; i++)
			collect_tree_free_keep_bitsets(root->children[i]);
		//itemtree_free_keep_bitsets(root->tree);
		itemtree_free(root->tree);
		free(root);
	}
}
*/

void collect_tree_free(cubenode_t *root)
{
	int i;

	//printf("root=%s children=%s len=%d, tree=%s\n", root? "Y": "N", root->children? "Y": "N", root->len, root->tree? "Y": "N");
	if (root->children) // leaf nodes should not be freed
	{
		for (i=0; i<root->len; i++)
			collect_tree_free(root->children[i]);
		itemtree_free(root->tree);
		free(root->children);
		free(root);
	}
}


void cubenode_print(cubenode_t *p)
{
	DEBUG("C(%d,%s%s%s%s)\n", p->len, p->tree? "T": " ", p->bitsets? "B": " ", p->itemsets? "I": " ", p->visited? "V": " ");
}

void cubetree_print(cubenode_t *root, int pad)
{
	int i;
	for (i=0; i<pad; i++)
		DEBUG(" ");
	cubenode_print(root);
	for (i=0; i<root->len; i++)
		cubetree_print(root->children[i], pad+1);
}

long dig_support(cubenode_t *root, itemset_t *itemset, long target_sup)
{
	// TODO: add (cache) dug node to the tree
	
	int i, j;
	int diglen;
	long child_exact_sum, achieved_sup;
	
	digs_cnt++;
	//printf("==== digging ");
	//itemset_print(itemset);
	//printf(" from %s target %ld\n", root->name, target_sup);
	
	if (root->len == 0)
	{
		//printf("dig reached leaf\n");
		leafdigs_cnt++;
		return bitset_bag_get_support(itemset, root->bitsets);
	}
	else
	{
		cubenode_t **dignodes = (cubenode_t **)malloc(sizeof(cubenode_t *)*root->len);
		diglen = 0;
		for (i=0, child_exact_sum=0; i<root->len; i++)
		{
			cubenode_t *child = root->children[i];
			itemnode_t *p, *chtree = child->tree;
			
			for(j=0, p=chtree; p!=NULL && j<itemset->len; j++, p=p->down)
			{
				while(p!=NULL && p->item!=itemset->items[j])
					p = p->right;
				if (p==NULL || j==itemset->len-1)
					break;
			}
			
			if (p!=NULL && j==itemset->len-1 && IS_EX(p->count))
				child_exact_sum += p->count;
			else
			{
				dignodes[diglen++] = child;
				//printf("will dig %s\n", child->name);
			}
		}
		//printf("child_exact_sum: %ld\n", child_exact_sum);
		
		achieved_sup = child_exact_sum;
		for (i=0; i<diglen && !GTE(achieved_sup, target_sup); i++)
		{
			long new_target = SUB(target_sup, achieved_sup);
			//printf("digging %s new target %ld\n", dignodes[i]->name, new_target);
			long new_achieve = dig_support(dignodes[i], itemset, new_target);
			achieved_sup = ADD(achieved_sup, new_achieve);
			//printf("achieved_sup: %ld\n", achieved_sup);
		}
		free(dignodes);
		return (i==diglen)? achieved_sup: TO_LB(achieved_sup);
	}
}

void space(int i)
{
	while (i-->0)
		printf(" ");
}

itemnode_t *merge_rec(cubenode_t *cube, itemnode_t **subtrees, int level)
{
	int i, yes;
	long sup;
	long minsup = SUP(cube->minsup, cube->items);
	itemnode_t *minm, *p;
	itemnode_t *head, *tail;
	
	//space(level); printf("enter merge request at level %d\n", level);
		
	head = NULL;
	while(1)
	{
		for (minm=NULL, i=0; i<cube->len; i++)
			if (subtrees[i] != NULL)
			{
				minm = subtrees[i];
				break;
			}
		if (minm==NULL)
		{
			//space(level); printf("level %d finished\n", level);
			return head;
		}		
		for (i++; i<cube->len; i++)
			if (subtrees[i] != NULL && subtrees[i]->item < minm->item)
				minm = subtrees[i];
		if (minm==NULL)
		{
			//space(level); printf("level %d finished\n", level);
			return head;
		}
		for (sup=0, i=0; i<cube->len; i++)
		{
			if (subtrees[i] != NULL && subtrees[i]->item == minm->item)
				sup = ADD(sup, subtrees[i]->count);
			else
				sup = TO_LB(sup);
		}
		//space(level); printf("level %d next item: %d (%ld)\n", level, minm->item, sup);
		
		if (!GTE(sup, minsup))
		{
			//space(level); printf("level %d digging %d target %ld\n", level, minm->item, minsup);
			itemset_t *path = (itemset_t *)malloc(sizeof(itemset_t));
			
			// level is sometimes one more than path len. use method below
			path->len = level;
			//for (path->len=0, p=minm; p!=NULL; p=p->up)
			//	path->len++;
			
			path->items = (int *)malloc(sizeof(int)*path->len);
			for (i=path->len-1, p=minm; p!=NULL && i>=0; p=p->up, i--)
				path->items[i] = p->item;
			
			//printf("%d >>>>", path->len);
			//for (p=minm; p!=NULL; p=p->up)
			//	printf("%d ", p->item);
			//printf("\n");
			
			sup = dig_support(cube, path, minsup);
			//space(level); printf("level %d digged %ld\n", level, sup);
		}
		
		if (GTE(sup, minsup))
		{
			itemnode_t *node = (itemnode_t *)malloc(sizeof(itemnode_t));
			node->item = minm->item;
			node->count = sup;
			node->right = NULL;
			node->up = NULL;
			node->bitset = NULL;
			if (head)
			{
				tail->right = node;
				tail = tail->right;
			}
			else
			{
				head = node;
				tail = node;
			}

			itemnode_t **subsub = (itemnode_t **)malloc(sizeof(itemnode_t *)*cube->len);
			for (yes=0, i=0; i<cube->len; i++)
			{
				subsub[i] = (subtrees[i]!=NULL && subtrees[i]->item==minm->item)? subtrees[i]->down: NULL;
				if (subsub[i])
					yes=1;
			}
			node->down = yes? merge_rec(cube, subsub, level+1): NULL;
			///this was a bug leaving right siblings orphan
			///if (node->down)
			///	node->down->up = node;
			///correction:
			for (p=node->down; p!=NULL; p=p->right)
				p->up = node;
			free(subsub);
		}
		
		for (i=0; i<cube->len; i++)
		{
			if (subtrees[i]!=NULL && subtrees[i]->item==minm->item)
				subtrees[i] = subtrees[i]->right;
		}
	}
}

itemnode_t *get_frequent(cubenode_t *root)
{
	//printf("enter get frequent on cube %s minsup %lu\n", root->name, SUP(root->minsup, root->items));
	if (root->tree == NULL)
	{
		int i;
		itemnode_t **subtrees = (itemnode_t **)malloc(sizeof(itemnode_t *)*root->len);
		for (i=0; i<root->len; i++)
			subtrees[i] = get_frequent(root->children[i]);
		root->tree = merge_rec(root, subtrees, 1);
		free(subtrees);
	}
	//printf("exit get frequent on cube %s minsup %lu\n", root->name, SUP(root->minsup, root->items));
	return root->tree;
}
