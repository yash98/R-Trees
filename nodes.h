#ifndef NODES_H
#define NODES_H

#include "file_manager.h"

extern int dimensionalityGlobal;
extern int maxCapGlobal;

enum NodeType{internal, leaf};

// point can be seen as a dimensionality dimension tuple
class internalNode {
	/*
	Page Memory Layout
	1.
	*/
public:
	int selfId;
	// Pointers to the corner points of the hyper rectangle
	// 0. Lower bounds
	// 1. Upper bounds
	int * mbr[2];
	
	// -1 if root
	int * parentId;

	int * numChilds;

	// maxCap number of child ids
	int * childIds;

	// MBR (lower bound and upper bound i.e. [2]) of maxCap children (first *) and child has dimensionality dimensions (second *)
	// Accessing i th dimension of MBR's upper bound of 3rd child *((*((childMBRs+2)[1]))+i)
	// first de-reference to get to i th point
	// second de-reference to get the j th dimension 
	int * childMBRs[2];

	internalNode(PageHandler page);
	~internalNode();
};

class leafNode {
	/* 
	Page Memory Layout
	1. parentId = 1 int
	2. numPoints = 1 int
	3. mbr = 2 * dimensionality int
	4. points = maxCap * dimensionality int
		1. dimensionality ints contigious for first point first
		2. now data for second int
	*/
public:
	int selfId;
	int * mbr[2];

	int * parentId;

	int * numPoints;

	// maxCap number of dimensionality dimension points
	// first de-reference to get to i th point
	// second de-reference to get the j th dimension 
	int * containedPoints;

	leafNode(PageHandler page);
	~leafNode();

	void insertPoint(int * point);
	
	void resetMBR();
};

#endif
