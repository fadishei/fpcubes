#ifndef LBOUND_H
#define LBOUND_H

#define IS_LB(x)	((x)<0)
#define IS_EX(x)	((x)>=0)
#define TO_LB(x)	((x)<0?(x):(~(x)))
#define TO_EX(x)	((x)<0?(~(x)):(x))
#define ADD(x, y)	((IS_LB(x)||IS_LB(y))?(~(TO_EX(x)+TO_EX(y))):((x)+(y)))
#define SUB(x, y)	((IS_LB(x)||IS_LB(y))?(~(TO_EX(x)-TO_EX(y))):((x)-(y)))
#define GTE(x, y)	((TO_EX(x))>=(TO_EX(y)))

#endif