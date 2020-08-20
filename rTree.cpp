#include "rTree.h"
#include "nodes.h"

#include <fstream>
#include <iostream>
#include <cstring>

int dimensionality;
int maxCap;

rTree::rTree(int dimensionality, int maxCap,  const char * treeFileName) {
	this->dimensionality = dimensionality;
	this->maxCap = maxCap;
	this->rootId = -1;

	this->rTreeFile = this->rTreeFileManager.CreateFile(treeFileName);
	this->rTreeFile.NewPage();

	this->writeInfoPage();
}

void rTree::writeInfoPage() {
	char * data = this->rTreeFile.FirstPage().GetData();

	memcpy(&data[0], &(this->rootId), sizeof(int));
	memcpy(&data[1*sizeof(int)], &(this->dimensionality), sizeof(int));
	memcpy(&data[2*sizeof(int)], &(this->maxCap), sizeof(int));

	this->rTreeFile.MarkDirty(0);
}

void rTree::bulkLoad(const char * filename, int numPoints) {

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
