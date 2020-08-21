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

	rTree(int dimensionality, int maxCap, const char * treeFileName);
	~rTree();

	void bulkLoad(const char * filename, int numPoints);
	int insert(const int * point);
	int query(const int * point);

private:
	FileHandler rTreeFile;
	FileManager rTreeFileManager;

	void setGlobals();

	int assignParent(int start, int end, NodeType nodeIs);
};


#endif
