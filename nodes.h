#ifndef NODES_H
#define NODES_H

#include "file_manager.h"

extern int dimensionalityGlobal;
extern int maxCapGlobal;

enum NodeType{internal, leaf};

NodeType TypeOf(PageHandler page);

// point can be seen as a dimensionality dimension tuple
class internalNode {
	/*
	Page Memory Layout
	1. enum - type of node
	2. parentId = 1 int
	3. numChilds = 1 int
	4. mbr = 2 * dimensionality int
	5. childIds = maxCap int
	6. childMBRs
		1. First, first childs' MBR[0] in contigious memory
		2. Then, second child's MBR[0] till maxCap number of children
		3. when MBR[0] is exhausted, MBR[1] section for child start
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

	int * childMBRs[2];

	internalNode(PageHandler page);
	// ~internalNode();

	void initPageData(PageHandler page);

	void insertNode(PageHandler childPage);
};

class leafNode {
	/* 
	Page Memory Layout
	1. enum - type of node
	2. parentId = 1 int
	3. numPoints = 1 int
	4. mbr = 2 * dimensionality int
	5. points = maxCap * dimensionality int
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
	// ~leafNode();

	void initPageData(PageHandler page);

	void insertPoint(int * point);
};

#endif
