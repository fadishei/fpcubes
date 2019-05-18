BITSET ?= ROARING
MEMPROF ?= 0
DBGLEVEL ?= 0

OBJXXS :=
OBJS := util.o itemset.o bitset.o itemtree.o eclat.o cube.o fpc.o
CFLAGS := -g -Wno-unused-result -DBITSET=$(BITSET) -DDBGLEVEL=$(DBGLEVEL)
CXXFLAGS := -g -std=c++11 -DBITSET=$(BITSET) -DDBGLEVEL=$(DBGLEVEL)
LDFLAGS := -no-pie -lm
OUT := fpc

ifeq ($(BITSET),ROARING)
	OBJS += roaring/roaring.o wrapper_roaring.o
	CFLAGS += -Iroaring
else ifeq ($(BITSET),EWAH)
	OBJXXS += wrapper_ewah.opp
	CXXFLAGS += -Iewah
else ifeq ($(BITSET),BM)
	OBJXXS += wrapper_bm.opp
	CXXFLAGS += -Ibm
else ifeq ($(BITSET),CONCISE)
	OBJXXS += wrapper_concise.opp
	CXXFLAGS += -Iconcise
endif

ifeq ($(MEMPROF),1)
	CFLAGS += -DMEMPROF
	LDFLAGS += -ltcmalloc
endif

default: $(OUT)

all: default

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

%.opp: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(OUT): $(OBJS) $(OBJXXS)
	$(CXX) $^ $(LDFLAGS) -o $@

clean:
	rm -f $(OBJS) $(OBJXXS) $(OUT)

