#include "nodes.h"

#include <exception>
#include <cstring>
#include <limits>
#include <algorithm>

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
		memcpy(this->childMBRs[0]+(*this->numChilds), child.mbr[0], sizeof(int) * dimensionalityGlobal);
		memcpy(this->childMBRs[1]+(*this->numChilds), child.mbr[1], sizeof(int) * dimensionalityGlobal);

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
		memcpy(this->childMBRs[0]+(*this->numChilds), child.mbr[0], sizeof(int) * dimensionalityGlobal);
		memcpy(this->childMBRs[1]+(*this->numChilds), child.mbr[1], sizeof(int) * dimensionalityGlobal);

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

void leafNode::insertPoint(int * point) {
	if (*numPoints >= maxCapGlobal) {
		throw LeafNodeFullException();
	}
	
	memcpy(containedPoints + (*this->numPoints), this->numPoints, sizeof(int) * dimensionalityGlobal);

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
