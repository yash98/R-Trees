#ifndef RTREE_H
#define RTREE_H

#include "file_manager.h"

class rTree
{
public:
	int dimensionality;
	int maxCap;

	rTree(int dimensionality, int maxCap, const char * treeFileName);
	~rTree();

	void bulkLoad(const char * filename, int numPoints);
	int insert(const int * point);
	int query(const int * point);

private:
	int rootId;

	FileHandler rTreeFile;
	FileManager rTreeFileManager;

	void writeInfoPage();
};


#endif
