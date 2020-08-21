#include "nodes.h"

#include <exception>
#include <cstring>
#include <limits>

struct LeafNodeFullException : public std::exception {
  const char *what () const throw () {
    return "Leaf node already has maxCap points, spliting should have been called";
  }
};


leafNode::leafNode(PageHandler page) {
	int * data = (int *) page.GetData();

	this->selfId = page.GetPageNum();
	this->parentId = data;
	this->numPoints = data+1;
	this->mbr[0] = data+2;
	this->mbr[1] = data+2+dimensionalityGlobal;
	this->containedPoints = data+2+(2*dimensionalityGlobal);
}

void leafNode::insertPoint(int * point) {
	if (*numPoints >= maxCapGlobal) {
		throw LeafNodeFullException();
	}
	
	memcpy(containedPoints + (*numPoints), numPoints, sizeof(int) * dimensionalityGlobal);

	*numPoints += 1;

	// Update MBR
	for (int i = 0; i < dimensionalityGlobal; i++) {
		if (*(this->mbr[0]+i) > *(point+i)) {
			*(this->mbr[0]+i) = *(point+i);

		}

		if (*(this->mbr[1]+i) < *(point+i)) {
			*(this->mbr[1]+i) = *(point+i);
		}
	}
}

void leafNode::resetMBR() {
	for (int i = 0; i < dimensionalityGlobal; i++) {
		*(this->mbr[0]+i) = std::numeric_limits<int>::max();
		*(this->mbr[1]+i) = std::numeric_limits<int>::min();
	}
}
