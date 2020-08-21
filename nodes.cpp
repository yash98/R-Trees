#include "nodes.h"

#include <exception>
#include <cstring>
#include <limits>
#include <algorithm>
#include <iostream>

int pointEquality(const int * a, const int * b) { 
	
	std::cout << "A ";
	for (int i = 0; i < dimensionalityGlobal; i++) {
		std::cout << *(a+i) << " ";
	}

	std::cout << "B ";
	for (int i = 0; i < dimensionalityGlobal; i++) {
		std::cout << *(b+i) << " ";
	}

	for (int i = 0; i < dimensionalityGlobal; i++) {
		if (*(a+i) != *(b+i)) {
			std::cout << "FALSE" << std::endl;
			return 0;
		}
	}
	std::cout << "TRUE" << std::endl;
	return 1;
}

int containedIn(const int * mbr[2], const int* p) {
	std::cout << "MBR0 ";
	for (int i = 0; i < dimensionalityGlobal; i++) {
		std::cout << *(mbr[0]+i) << " ";
	}

	std::cout << "MBR1 ";
	for (int i = 0; i < dimensionalityGlobal; i++) {
		std::cout << *(mbr[1]+i) << " ";
	}
	
	std::cout << "P ";
	for (int i = 0; i < dimensionalityGlobal; i++) {
		std::cout << *(p+i) << " ";
	}
	
	for (int i = 0; i < dimensionalityGlobal; i++) {
		if ((*(p+i) < *(mbr[0]+i)) || (*(mbr[1]+i) < *(p+i))) {
			std::cout << "FALSE" << std::endl;
			return 0;
		}
	}
	
	std::cout << "TRUE" << std::endl;
	return 1;
}

struct InvalidAccessException : public std::exception {
  const char *what () const throw () {
    return "Access of page 0 is Denied";
  }
};

NodeType TypeOf(PageHandler page) {
	if (page.GetPageNum() == 0) throw InvalidAccessException();
	int * data = (int *) page.GetData();
	return (NodeType) *data;
}

struct InternalNodeFullException : public std::exception {
  const char *what () const throw () {
    return "Internal node already has maxCap childs, spliting should have been called";
  }
};

internalNode::internalNode(PageHandler page) {
	int * data = (int *) page.GetData();

	this->selfId = page.GetPageNum();
	// First space occupied by enum, hence data here is offset by +1 even at the start
	this->parentId = data+1;
	this->numChilds = data+2;
	this->mbr[0] = data+3;
	this->mbr[1] = data+3+dimensionalityGlobal;
	this->childIds = data+3+(2*dimensionalityGlobal);
	this->childMBRs[0] = data+3+(2*dimensionalityGlobal)+maxCapGlobal;
	this->childMBRs[1] = data+3+(2*dimensionalityGlobal)+maxCapGlobal+(maxCapGlobal*dimensionalityGlobal);
}

void internalNode::initPageData(PageHandler page) {
	int * data = (int *) page.GetData();
	
	*data = (int) internal;
	*this->numChilds = 0;
	*this->parentId = -1;

	for (int i = 0; i < dimensionalityGlobal; i++) {
		*(this->mbr[0]+i) = std::numeric_limits<int>::max();
		*(this->mbr[1]+i) = std::numeric_limits<int>::min();
	}
}

void internalNode::insertNode(PageHandler childPage) {
	if (*numChilds >= maxCapGlobal) {
		throw InternalNodeFullException();
	}

	NodeType nodeIs = TypeOf(childPage);
	
	if (nodeIs == internal) {
		internalNode child(childPage);
		*(this->childIds + (*this->numChilds)) = child.selfId;

		// set ChildMBRs
		memcpy(this->childMBRs[0]+(*this->numChilds * dimensionalityGlobal), child.mbr[0], sizeof(int) * dimensionalityGlobal);
		memcpy(this->childMBRs[1]+(*this->numChilds * dimensionalityGlobal), child.mbr[1], sizeof(int) * dimensionalityGlobal);

		// Update mbr
		for (int i = 0; i < dimensionalityGlobal; i++) {
			if (*(this->mbr[0]+i) > *(child.mbr[0]+i)) {
				*(this->mbr[0]+i) = *(child.mbr[0]+i);
			}

			if (*(this->mbr[1]+i) < *(child.mbr[1]+i)) {
				*(this->mbr[1]+i) = *(child.mbr[1]+i);
			}
		}

		// ParentId update for child
		*(child.parentId) = this->selfId;

	} else {
		leafNode child(childPage);
		*(this->childIds + (*this->numChilds)) = child.selfId;

		// set ChildMBRs
		memcpy(this->childMBRs[0]+(*this->numChilds * dimensionalityGlobal), child.mbr[0], sizeof(int) * dimensionalityGlobal);
		memcpy(this->childMBRs[1]+(*this->numChilds * dimensionalityGlobal), child.mbr[1], sizeof(int) * dimensionalityGlobal);

		// Update mbr
		for (int i = 0; i < dimensionalityGlobal; i++) {
			if (*(this->mbr[0]+i) > *(child.mbr[0]+i)) {
				*(this->mbr[0]+i) = *(child.mbr[0]+i);
			}

			if (*(this->mbr[1]+i) < *(child.mbr[1]+i)) {
				*(this->mbr[1]+i) = *(child.mbr[1]+i);
			}
		}

		// ParentId update for child
		*(child.parentId) = this->selfId;
	}

	*this->numChilds += 1;
}

struct LeafNodeFullException : public std::exception {
  const char *what () const throw () {
    return "Leaf node already has maxCap points, spliting should have been called";
  }
};

leafNode::leafNode(PageHandler page) {
	int * data = (int *) page.GetData();

	this->selfId = page.GetPageNum();
	// First space occupied by enum
	this->parentId = data+1;
	this->numPoints = data+2;
	this->mbr[0] = data+3;
	this->mbr[1] = data+3+dimensionalityGlobal;
	this->containedPoints = data+3+(2*dimensionalityGlobal);
}

//  Fully correct
void leafNode::insertPoint(int * point) {
	if (*numPoints >= maxCapGlobal) {
		throw LeafNodeFullException();
	}

	// std::cout << "I ";
	// for (int i = 0; i < dimensionalityGlobal; i++) {
	// 	std::cout << *(point+i) << " ";
	// }
	// std::cout << std::endl;
	
	memcpy(this->containedPoints + (dimensionalityGlobal * (*this->numPoints)), point, sizeof(int) * dimensionalityGlobal);

	// std::cout << "C ";
	// for (int i = 0; i < dimensionalityGlobal; i++) {
	// 	std::cout << *((this->containedPoints + (dimensionalityGlobal * (*this->numPoints)))+i) << " ";
	// }
	// std::cout << std::endl;


	// Update MBR
	for (int i = 0; i < dimensionalityGlobal; i++) {
		if (*(this->mbr[0]+i) > *(point+i)) {
			*(this->mbr[0]+i) = *(point+i);

		}

		if (*(this->mbr[1]+i) < *(point+i)) {
			*(this->mbr[1]+i) = *(point+i);
		}
	}

	*this->numPoints += 1;
}

void leafNode::initPageData(PageHandler page) {
	int * data = (int *) page.GetData();
	
	*data = (int) leaf;
	*this->numPoints = 0;
	*this->parentId = -1;

	for (int i = 0; i < dimensionalityGlobal; i++) {
		*(this->mbr[0]+i) = std::numeric_limits<int>::max();
		*(this->mbr[1]+i) = std::numeric_limits<int>::min();
	}
}
