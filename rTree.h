#ifndef RTREE_H
#define RTREE_H

#include "file_manager.h"

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
};


#endif
