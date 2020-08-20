#ifndef NODES_H
#define NODES_H

extern int dimensionality;
extern int maxCap;

enum NodeType{internal, leaf};

// point can be seen as a dimensionality dimension tuple
class internalNode {
public:
	int selfId;
	// Pointers to the corner points of the hyper rectangle
	// 0. Lower bounds
	// 1. Upper bounds
	int * mbr[2];
	
	// -1 if root
	int parentId;

	// maxCap number of child ids
	int * childIds;

	// MBR (lower bound and upper bound i.e. [2]) of maxCap children (first *) and child has dimensionality dimensions (second *)
	// Accessing i th dimension of MBR's upper bound of 3rd child *((*((childMBRs+2)[1]))+i)
	// first de-reference to get to i th point
	// second de-reference to get the j th dimension 
	int ** childMBRs[2];

	internalNode(int pageNum);
	~internalNode();
};

class leafNode {
public:
	int selfId;
	int * mbr[2];

	int parentId;

	// maxCap number of dimensionality dimension points
	// first de-reference to get to i th point
	// second de-reference to get the j th dimension 
	int ** containedPoints;

	leafNode(int pageNum);
	~leafNode();
};

#endif
