#include "rTree.h"
#include "constants.h"

#include <fstream>
#include <iostream>
#include <cstring>
#include <cmath>
#include <limits>

int dimensionalityGlobal;
int maxCapGlobal;

rTree::rTree(int dimensionalityInput, int maxCapInput, const char * treeFileName) {
	this->rTreeFile = this->rTreeFileManager.CreateFile(treeFileName);
	this->rTreeFile.NewPage();

	int * data = (int *) this->rTreeFile.FirstPage().GetData();
	
	int tempRootId = -1;
	memcpy(data, &tempRootId, sizeof(int));
	this->rootId = data;

	memcpy(data + 1, &dimensionalityInput, sizeof(int));
	this->dimensionality = data + 1;

	memcpy(data + 2, &maxCapInput, sizeof(int));
	this->maxCap = data + 2;

	this->rTreeFile.MarkDirty(0);
}

void rTree::setGlobals() {
	dimensionalityGlobal = *(this->dimensionality);
	maxCapGlobal = *(this->maxCap);
}

void rTree::bulkLoad(const char * filename, int numPoints) {
	this->setGlobals();

	FileHandler dataPointFile = this->rTreeFileManager.OpenFile(filename);

	int pointFromPage = 0;
	int maxPointFromPage = floor(PAGE_CONTENT_SIZE/(sizeof(int) * dimensionalityGlobal));

	int currentPage = 0;
	int pointCount = 0;

	int * data = (int *) dataPointFile.PageAt(currentPage).GetData();

	int endLeafPage;

	// Instead of number of slab condition
	while (pointCount < numPoints) {
		PageHandler leafPage = this->rTreeFile.NewPage();
		endLeafPage = leafPage.GetPageNum();

		leafNode currentLeaf(leafPage);
		currentLeaf.initPageData(leafPage);
		

		int pointsLeft = numPoints-pointCount;
		// Writing points to leaf
		for (int i=0; i < std::min(maxCapGlobal, pointsLeft); i++) {

			// Page changing condition
			if (pointFromPage >= maxPointFromPage) {
				dataPointFile.UnpinPage(currentPage);
				data = (int *) dataPointFile.PageAt(currentPage++).GetData();
				pointFromPage = 0;
			}

			// Insert Point
			std::cout << "B ";
			for (int j = 0; j < dimensionalityGlobal; j++) {
				std::cout << *(data+j) << " ";
			}
			std::cout << std::endl;

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

	this->rTreeFileManager.CloseFile(dataPointFile);

	*(this->rootId) = assignParent(1, endLeafPage);
}

int rTree::assignParent(int start, int end) {
	int newStart = end + 1;
	int newEnd;

	int currentChildPage = start;

	while (currentChildPage <= end) {
		// Parent Node creation
		PageHandler parentPage = this->rTreeFile.NewPage();
		newEnd = parentPage.GetPageNum();

		internalNode parentNode(parentPage);
		parentNode.initPageData(parentPage);
		int leftNodes = std::min(maxCapGlobal, end-currentChildPage+1);
		for (int i=0; i< leftNodes; i++) {
			parentNode.insertNode(this->rTreeFile.PageAt(currentChildPage));

			this->rTreeFile.UnpinPage(currentChildPage);
			currentChildPage++;
		}

		this->rTreeFile.MarkDirty(newEnd);
	}

	//  Base case for root node
	if (start - end <= maxCapGlobal) {
		this->rTreeFile.UnpinPage(newStart);
		return newStart;
	}

	return assignParent(newStart, newEnd);
}

void rTree::insert(int * point) {
	int selectedLeafId = selectLeaf(*this->rootId, point);

	PageHandler selectedLeafPage = this->rTreeFile.PageAt(selectedLeafId);
	leafNode selectedLeaf(selectedLeafPage);

	//  This handle all the insertion for leaf
	if (*selectedLeaf.numPoints < maxCapGlobal) {
		selectedLeaf.insertPoint(point);
		// this->rTreeFile.UnpinPage(selectedLeafId);
		propagateUp(*selectedLeaf.parentId, 0, selectedLeafId, -1, selectedLeaf.mbr, -1);
		this->rTreeFile.MarkDirty(selectedLeafId);
		this->rTreeFile.UnpinPage(selectedLeafId);
	} else {
		// Spliting
		// Quadratic Split
		int * point1, * point2;
		int bestChildId1, bestChildId2;

		// Seeds picked
		long long maxDistance = std::numeric_limits<long long>::min();
		for (int i=0; i < maxCapGlobal+1; i++) {
			point1 = selectedLeaf.containedPoints+ (i*dimensionalityGlobal);
			for (int j=i+1; j < maxCapGlobal+1; j++) {
				point2 = selectedLeaf.containedPoints+ (j*dimensionalityGlobal);
				long long currentDistance = distance(point1, point2);

				if (currentDistance > maxDistance) {
					maxDistance = currentDistance;
					bestChildId1 = i;
					bestChildId2 = j;
				}
			}
		}

		PageHandler leafPage1 = this->rTreeFile.NewPage();
		leafNode splittedLeaf1(leafPage1);
		splittedLeaf1.initPageData(leafPage1);

		splittedLeaf1.insertPoint(selectedLeaf.containedPoints+(bestChildId1*dimensionalityGlobal));
		
		PageHandler leafPage2 = this->rTreeFile.NewPage();
		leafNode splittedLeaf2(leafPage2);
		splittedLeaf2.initPageData(leafPage2);

		splittedLeaf2.insertPoint(selectedLeaf.containedPoints+(bestChildId2*dimensionalityGlobal));

		// Assign others, pick next
		for (int i=0; i < maxCapGlobal+1; i++) {
			if ((i != bestChildId1) || (i != bestChildId2)) {
				int * currentPoint = selectedLeaf.containedPoints + (i*dimensionalityGlobal);
				if (*splittedLeaf1.numPoints == maxCapGlobal) {
					splittedLeaf2.insertPoint(currentPoint);
					continue;
				}
				if (*splittedLeaf2.numPoints == maxCapGlobal) {
					splittedLeaf1.insertPoint(currentPoint);
					continue;
				}
				
				long long areaEnlarged1, areaEnlarged2;

				areaEnlarged1 = areaEnlargement(splittedLeaf1.mbr, currentPoint);
				areaEnlarged2 = areaEnlargement(splittedLeaf2.mbr, currentPoint);

				if (areaEnlarged1 < areaEnlarged2) {
					splittedLeaf1.insertPoint(currentPoint);
				} else if (areaEnlarged1 == areaEnlarged2) {
					if (area(splittedLeaf1.mbr) < area(splittedLeaf2.mbr)) {
						splittedLeaf1.insertPoint(currentPoint);
					} else {
						splittedLeaf1.insertPoint(currentPoint);
					}
				} else {
					splittedLeaf2.insertPoint(currentPoint);
				}
			}
		}

		propagateUp(*selectedLeaf.parentId, 1, splittedLeaf1.selfId, splittedLeaf2.selfId, nullptr, selectedLeafId);

		this->rTreeFile.MarkDirty(splittedLeaf1.selfId);
		this->rTreeFile.UnpinPage(splittedLeaf1.selfId);

		this->rTreeFile.MarkDirty(splittedLeaf2.selfId);
		this->rTreeFile.UnpinPage(splittedLeaf2.selfId);

		this->rTreeFile.DisposePage(selectedLeafId);
	}

}

void rTree::propagateUp(int pageId, int didItSplit, int child1, int child2, int * childsUpdatedMBR[2], int prevChildId) {
	PageHandler selectedPage = this->rTreeFile.PageAt(pageId);
	internalNode selectedNode(selectedPage);

	int isRoot = 0;
	if (*selectedNode.parentId == -1) isRoot = 1;

	if (didItSplit) {
		PageHandler childPage1 = this->rTreeFile.PageAt(child1);
		internalNode childNode1(childPage1);
		
		PageHandler childPage2 = this->rTreeFile.PageAt(child2);
		internalNode childNode2(childPage2);

		if (*selectedNode.numChilds < maxCapGlobal) {
			// Find index of previously existing child
			int foundIndex = selectedNode.findChild(prevChildId);

			selectedNode.replaceChild(foundIndex, childPage1);
			selectedNode.insertNode(childPage2);
			
			*(childNode1.parentId) = pageId;
			*(childNode2.parentId) = pageId;

			if (!isRoot) propagateUp(*selectedNode.parentId, 0, selectedNode.selfId, -1, selectedNode.mbr, -1);
			this->rTreeFile.MarkDirty(pageId);
			this->rTreeFile.UnpinPage(pageId);
		} else {
			// pick seed
			int bestChildId1, bestChildId2;
			long long minGroupingInefficiency = std::numeric_limits<long long>::max();
			for (int i=0; i < maxCapGlobal+1; i++) {
				PageHandler seedChildPage1 = this->rTreeFile.PageAt(*(selectedNode.childIds+i));
				internalNode seedChild1(seedChildPage1);
				for (int j=i+1; j < maxCapGlobal+1; j++) {
					PageHandler seedChildPage2 = this->rTreeFile.PageAt(*(selectedNode.childIds+j));
					internalNode seedChild2(seedChildPage2);

					long long currentGroupInnefficiency = groupingInefficiency(seedChild1.mbr, seedChild2.mbr);
					if (minGroupingInefficiency > currentGroupInnefficiency) {
						minGroupingInefficiency = currentGroupInnefficiency;
						bestChildId1 = seedChild1.selfId;
						bestChildId2 = seedChild2.selfId;
					}
				}
			}

			// Assign others
			PageHandler nodePage1 = this->rTreeFile.NewPage();
			internalNode splittedNode1(nodePage1);
			splittedNode1.initPageData(nodePage1);

			splittedNode1.insertNode(this->rTreeFile.PageAt(bestChildId1));

			PageHandler nodePage2 = this->rTreeFile.NewPage();
			internalNode splittedNode2(nodePage2);
			splittedNode2.initPageData(nodePage2);

			splittedNode2.insertNode(this->rTreeFile.PageAt(bestChildId2));

			for (int i=0; i < maxCapGlobal+1; i++) {
				if ((i != bestChildId1) || (i != bestChildId2)) {
					PageHandler candidateNodePage = this->rTreeFile.PageAt(*(selectedNode.childIds + i));
					internalNode candidateNode(candidateNodePage);

					if (*splittedNode1.numChilds == maxCapGlobal) {
						splittedNode2.insertNode(candidateNodePage);
						continue;
					}
					if (*splittedNode2.numChilds == maxCapGlobal) {
						splittedNode1.insertNode(candidateNodePage);
						continue;
					}

					long long groupIneff1, groupIneff2, candidateArea;

					candidateArea = area(candidateNode.mbr);
					groupIneff1 = groupingInefficiency(splittedNode1.mbr, candidateNode.mbr) + candidateArea;
					groupIneff2 = groupingInefficiency(splittedNode2.mbr, candidateNode.mbr) + candidateArea;

					if (groupIneff1 < groupIneff2) {
						splittedNode1.insertNode(candidateNodePage);
					} else if (groupIneff1 == groupIneff2) {
						if (area(splittedNode1.mbr) < area(splittedNode2.mbr)) {
							splittedNode1.insertNode(candidateNodePage);
						} else {
							splittedNode2.insertNode(candidateNodePage);
						}
					} else {
						splittedNode2.insertNode(candidateNodePage);
					}
				}
			}

			if (!isRoot) {
				propagateUp(*selectedNode.parentId, 1, splittedNode1.selfId, splittedNode2.selfId, nullptr, pageId);
			} else {
				PageHandler newRootPage = this->rTreeFile.NewPage();
				internalNode newRoot(newRootPage);
				newRoot.initPageData(newRootPage);

				newRoot.insertNode(nodePage1);
				newRoot.insertNode(nodePage2);

				*(splittedNode1.parentId) = newRoot.selfId;
				*(splittedNode2.parentId) = newRoot.selfId;

				*this->rootId = newRoot.selfId;
			}
			this->rTreeFile.MarkDirty(splittedNode1.selfId);
			this->rTreeFile.UnpinPage(splittedNode1.selfId);

			this->rTreeFile.MarkDirty(splittedNode2.selfId);
			this->rTreeFile.UnpinPage(splittedNode2.selfId);

			this->rTreeFile.DisposePage(pageId);
		}
	} else {
		// Update childs MBR saved in parents node
		int foundIndex = selectedNode.findChild(child1);

		memcpy(selectedNode.childMBRs[0] + (foundIndex*dimensionalityGlobal), childsUpdatedMBR[0], sizeof(int)*dimensionalityGlobal);
		memcpy(selectedNode.childMBRs[1] + (foundIndex*dimensionalityGlobal), childsUpdatedMBR[1], sizeof(int)*dimensionalityGlobal);

		// Update selfs mbr
		selectedNode.updateMBR(childsUpdatedMBR);

		if (!isRoot) propagateUp(*selectedNode.parentId, 0, selectedNode.selfId, -1, selectedNode.mbr, -1);
		this->rTreeFile.UnpinPage(selectedNode.selfId);
	}
}

int rTree::selectLeaf(int pageId, int * point) {
	PageHandler currentNodePage = this->rTreeFile.PageAt(pageId);
	NodeType nodeIs = TypeOf(currentNodePage);

	if (nodeIs == leaf) {
		return pageId;
	}

	internalNode currentNode(currentNodePage);

	int pickedChildIndex = 1;
	long long minAreaEnlarged = std::numeric_limits<long long>::max();
	long long tieBreakerMinArea = std::numeric_limits<long long>::max();

	for (int i=0; i < *currentNode.numChilds; i++) {
		int * childMBR[2] = {currentNode.childMBRs[0]+(i*dimensionalityGlobal), currentNode.childMBRs[1]+(i*dimensionalityGlobal)};
		long long currentAreaEnlarged = areaEnlargement(childMBR, point);

		if (currentAreaEnlarged < minAreaEnlarged) {
			minAreaEnlarged = currentAreaEnlarged;
			pickedChildIndex = i;
		} else if (currentAreaEnlarged == minAreaEnlarged) {
			long long currentArea = area(childMBR);
			if (currentArea < tieBreakerMinArea) {
				tieBreakerMinArea = currentArea;
				pickedChildIndex = i;
			}
		}
	}

	// Unpin when traversing up
	return selectLeaf(*(currentNode.childIds + pickedChildIndex), point);
}


int depth;

int rTree::query(int * point) {
	this->setGlobals();
	depth = 0;

	int found = dfs(*this->rootId, point);

	std::cout << "Q ";
	for (int i = 0; i < dimensionalityGlobal; i++) {
		std::cout << *(point+i) << " ";
	}


	std::cout << found << std::endl;
	return found;
}


int rTree::dfs(int pageId, int * point) {
	std::cout << depth << std::endl;
	depth++;

	PageHandler currentNodePage = this->rTreeFile.PageAt(pageId);
	NodeType nodeIs = TypeOf(currentNodePage);

	int found = 0;

	if (nodeIs == leaf) {
		leafNode currentNode(currentNodePage);
		for (int i=0; i < *currentNode.numPoints; i++) {
			if (pointEquality(currentNode.containedPoints+(i*dimensionalityGlobal), point)) {
				found = 1;
				break;
			}
		}
	} else {
		internalNode currentNode(currentNodePage);
		for (int i=0; i < *currentNode.numChilds; i++) {
			int * childMBR[2] = {currentNode.childMBRs[0]+(i*dimensionalityGlobal), currentNode.childMBRs[1]+(i*dimensionalityGlobal)};
			if (containedIn(childMBR, point)) {
				found = dfs(*(currentNode.childIds+i), point);
				if (found) break;
			}
		}
	}

	depth--;
	this->rTreeFile.UnpinPage(pageId);
	return found;
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
	
	std::string delimiter = "\n\n\n";

	while(queryFile >> queryType) {
		if (queryType == "BULKLOAD") {

			std::string bulkloadFileName;
			int numPoints;
			queryFile >> bulkloadFileName;
			queryFile >> numPoints;

			tree.bulkLoad(bulkloadFileName.c_str(), numPoints);
			outputFile << "BULKLOAD" << delimiter;

		} else if (queryType == "INSERT") {

			pointCopy();
			tree.insert(point);
			outputFile << "INSERT" << delimiter;

		} else if (queryType == "QUERY") {

			pointCopy();

			if (tree.query(point)) outputFile << "TRUE" << delimiter;
			else outputFile << "FALSE" << delimiter;

		} else {
			outputFile << "UNEXPECTED" << delimiter;
		}
	}

	queryFile.close();
	outputFile.close();

	tree.rTreeFileManager.ClearBuffer();
	tree.rTreeFileManager.DestroyFile("tree.txt");
};
