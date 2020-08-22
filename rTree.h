#ifndef RTREE_H
#define RTREE_H

#include "file_manager.h"
#include "nodes.h"


class rTree
{
public:
	int * rootId;
	int * dimensionality;
	int * maxCap;

	FileHandler rTreeFile;
	FileManager rTreeFileManager;

	rTree(int dimensionality, int maxCap, const char * treeFileName);
	// ~rTree();

	void bulkLoad(const char * filename, int numPoints);
	void insert(int * point);
	int query(int * point);

private:
	void setGlobals();
	int assignParent(int start, int end);
	int dfs(int pageId, int * point);
	int selectLeaf(int pageId, int * point);
	void propagateUp(int pageId, int didItSplit, int child1, int child2, int * childsUpdatedMBR[2], int prevChildId);
	void unpinAndFlush(int pageId);
};


#endif
