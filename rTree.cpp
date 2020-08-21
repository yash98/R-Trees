#include "rTree.h"
#include "nodes.h"
#include "constants.h"

#include <fstream>
#include <iostream>
#include <cstring>
#include <cmath>

int dimensionalityGlobal;
int maxCapGlobal;

rTree::rTree(int dimensionalityInput, int maxCapInput,  const char * treeFileName) {
	this->rTreeFile = this->rTreeFileManager.CreateFile(treeFileName);
	this->rTreeFile.NewPage();

	int * data = (int *) this->rTreeFile.FirstPage().GetData();
	
	int tempRootId = -1;
	memcpy((char *) data, &tempRootId, sizeof(int));
	this->rootId = data;

	memcpy((char *) (data + 1), &dimensionalityInput, sizeof(int));
	this->dimensionality = data + 1;

	memcpy((char *) (data + 2), &maxCapInput, sizeof(int));
	this->maxCap = data + 2;

	this->rTreeFile.MarkDirty(0);
	this->rTreeFile.UnpinPage(0);
}

rTree::~rTree() {
	std::cout << "rTree Destructor called" << std::endl;
}

void rTree::setGlobals() {
	dimensionalityGlobal = *(this->dimensionality);
	maxCapGlobal = *(this->maxCap);
}

void rTree::bulkLoad(const char * filename, int numPoints) {
	this->setGlobals();

	FileHandler dataPointFile = this->rTreeFileManager.OpenFile(filename);

	std::cout << numPoints << " number of points" << std::endl;

	int pointFromPage = 0;
	int maxPointFromPage = floor(PAGE_CONTENT_SIZE/(sizeof(int) * dimensionalityGlobal));

	int currentPage = 1;
	int pointCount = 0;

	int * data = (int *) dataPointFile.PageAt(currentPage).GetData();


	// Instead of number of slab condition
	while (pointCount < numPoints) {
		PageHandler leafPage = this->rTreeFile.NewPage();
		leafNode currentLeaf(leafPage);
		currentLeaf.resetMBR();
		
		// Writing points to leaf
		for (int i=0; i < std::min(maxCapGlobal, numPoints-pointCount); i++) {

			// Page changing condition
			if (pointFromPage >= maxPointFromPage) {
				dataPointFile.UnpinPage(currentPage);
				data = (int *) dataPointFile.PageAt(currentPage++).GetData();
				pointFromPage = 0;
			}

			// Insert Point
			currentLeaf.insertPoint(data);

			// Update data pointer for next point
			data += dimensionalityGlobal;
			pointFromPage++;
			pointCount++;
		}

		dataPointFile.MarkDirty(leafPage.GetPageNum());
		// TODO: Unpinning will happen in assignParent 
	}

	dataPointFile.UnpinPage(currentPage);
}

int rTree::insert(const int * point) {
	return 1;
}

int rTree::query(const int * point) {
	return 1;
}


int main(int argc, char * argv[]) {
	// ./rtree query.txt maxCap dimensionality output.txt
	int maxCapInput = atoi(argv[2]);
	int dimensionalityInput = atoi(argv[3]);
	
	std::ifstream queryFile;
	queryFile.open(argv[1]);

	std::ofstream outputFile;
	outputFile.open(argv[4]);

	std::string queryType;

	rTree tree(dimensionalityInput, maxCapInput, "tree.txt");

	int point[dimensionalityInput];

	auto pointCopy = [&queryFile, &point, &dimensionalityInput]() {for (int i = 0; i < dimensionalityInput; i++) queryFile >> point[i];};
	
	while(queryFile >> queryType) {
		if (queryType == "BULKLOAD") {

			std::string bulkloadFileName;
			int numPoints;
			queryFile >> bulkloadFileName;
			queryFile >> numPoints;

			tree.bulkLoad(bulkloadFileName.c_str(), numPoints);
			outputFile << "BULKLOAD" << std::endl;

		} else if (queryType == "INSERT") {

			pointCopy();
			tree.insert(point);
			outputFile << "INSERT" << std::endl;

		} else if (queryType == "QUERY") {

			pointCopy();

			if (tree.query(point)) outputFile << "TRUE" << std::endl;
			else outputFile << "FALSE" << std::endl;

		} else {
			outputFile << "UNEXPECTED" << std::endl;
		}
	}

	queryFile.close();
	outputFile.close();
};
