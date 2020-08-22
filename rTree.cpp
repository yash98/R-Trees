#include "rTree.h"
#include "constants.h"

#include <fstream>
#include <iostream>
#include <cstring>
#include <cmath>
#include <limits>

int dimensionalityGlobal;
int maxCapGlobal;

void rTree::unpinAndFlush(int pageId) {
	this->rTreeFile.UnpinPage(pageId);
	this->rTreeFile.FlushPage(pageId);
}

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
		int pieces = std::min(maxCapGlobal, pointsLeft);
		for (int i=0; i < pieces; i++) {

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

		this->rTreeFile.MarkDirty(leafPage.GetPageNum());
		this->rTreeFile.UnpinPage(leafPage.GetPageNum());
		this->rTreeFile.FlushPage(leafPage.GetPageNum());
	}
	dataPointFile.UnpinPage(currentPage);

	this->rTreeFileManager.CloseFile(dataPointFile);

	*(this->rootId) = assignParent(1, endLeafPage);
	this->rTreeFile.MarkDirty(0);
}

int rTree::assignParent(int start, int end) {
	int newStart = end + 1;
	int newEnd;

	int currentChildPageId = start;

	while (currentChildPageId <= end) {
		// Parent Node creation
		PageHandler parentPage = this->rTreeFile.NewPage();
		newEnd = parentPage.GetPageNum();

		internalNode parentNode(parentPage);
		parentNode.initPageData(parentPage);
		int leftNodes = std::min(maxCapGlobal, end-currentChildPageId+1);
		for (int i=0; i< leftNodes; i++) {
			PageHandler currentChildPage = this->rTreeFile.PageAt(currentChildPageId);
			genericNode currentChild(currentChildPage);

			parentNode.insertNode(currentChildPage);

			*currentChild.parentId = newEnd;

			this->rTreeFile.MarkDirty(currentChildPageId);
			this->unpinAndFlush(currentChildPageId);
			currentChildPageId++;
		}

		this->rTreeFile.MarkDirty(newEnd);
		this->unpinAndFlush(newEnd);
	}

	//  Base case for root node
	if (start - end <= maxCapGlobal) {
		return newStart;
	}

	return assignParent(newStart, newEnd);
}

void rTree::insert(int * point) {
	this->setGlobals();

	std::cout << "I ";
	for (int i = 0; i < dimensionalityGlobal; i++) {
		std::cout << *(point+i) << " ";
	}
	std::cout << std::endl;

	int selectedLeafId = selectLeaf(*this->rootId, point);

	PageHandler selectedLeafPage = this->rTreeFile.PageAt(selectedLeafId);
	leafNode selectedLeaf(selectedLeafPage);

	std::cout << "SELECTED " << selectedLeafId << std::endl; 

	//  This handle all the insertion for leaf
	if (*selectedLeaf.numPoints < maxCapGlobal) {
		std::cout << "LEAF INS NO SPLIT" << std::endl;

		selectedLeaf.insertPoint(point);

		propagateUp(*selectedLeaf.parentId, 0, selectedLeafId, -1, selectedLeaf.mbr, -1);

		this->rTreeFile.MarkDirty(selectedLeafId);
		this->unpinAndFlush(selectedLeafId);
	} else {
		std::cout << "LEAF INS SPLITTING" << std::endl;

		// Spliting
		// Quadratic Split
		int * bestChildPoint1, * bestChildPoint2;
		int bestChildId1, bestChildId2;

		// Seeds picked
		long long maxDistance = std::numeric_limits<long long>::min();
		for (int i=0; i < maxCapGlobal; i++) {
			int * point1 = selectedLeaf.containedPoints+ (i*dimensionalityGlobal);
			for (int j=i+1; j < maxCapGlobal+1; j++) {
				int * point2;
				if (j==maxCapGlobal) {
					point2 = point;
				} else {
					point2 = selectedLeaf.containedPoints+ (j*dimensionalityGlobal);
				}

				long long currentDistance = distance(point1, point2);

				if (currentDistance > maxDistance) {
					maxDistance = currentDistance;
					bestChildPoint1 = point1;
					bestChildPoint2 = point2;
					bestChildId1 = i;
					bestChildId2 = j;
				}
			}
		}

		PageHandler leafPage1 = this->rTreeFile.NewPage();
		leafNode splittedLeaf1(leafPage1);
		splittedLeaf1.initPageData(leafPage1);

		splittedLeaf1.insertPoint(bestChildPoint1);
		
		PageHandler leafPage2 = this->rTreeFile.NewPage();
		leafNode splittedLeaf2(leafPage2);
		splittedLeaf2.initPageData(leafPage2);

		splittedLeaf2.insertPoint(bestChildPoint2);

		// Assign others, pick next
		for (int i=0; i < maxCapGlobal+1; i++) {
			if ((i != bestChildId1) && (i != bestChildId2)) {
				int * currentPoint;
				if (i == maxCapGlobal) {
					currentPoint = point;
				} else {
					currentPoint = selectedLeaf.containedPoints + (i*dimensionalityGlobal);
				}

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

		int newPageId = *selectedLeaf.parentId;
		int newChild1 = splittedLeaf1.selfId;
		int newChild2 = splittedLeaf2.selfId;

		this->rTreeFile.MarkDirty(splittedLeaf1.selfId);
		this->rTreeFile.UnpinPage(splittedLeaf1.selfId);
		this->rTreeFile.FlushPage(splittedLeaf1.selfId);

		this->rTreeFile.MarkDirty(splittedLeaf2.selfId);
		this->rTreeFile.UnpinPage(splittedLeaf2.selfId);
		this->rTreeFile.FlushPage(splittedLeaf2.selfId);

		this->rTreeFile.DisposePage(selectedLeafId);
		this->rTreeFile.FlushPage(selectedLeafId);

		propagateUp(newPageId, 1, newChild1, newChild2, nullptr, selectedLeafId);
	}

}

void rTree::propagateUp(int pageId, int didItSplit, int child1, int child2, int * childsUpdatedMBR[2], int prevChildId) {
	PageHandler selectedPage = this->rTreeFile.PageAt(pageId);
	internalNode selectedNode(selectedPage);

	std::cout << "PROPAGATING: SELF " << pageId << " CHILDREN " << child1 << " " << child2 << std::endl;

	int isRoot = 0;
	if (*selectedNode.parentId == -1) isRoot = 1;

	if (didItSplit) {
		if (*selectedNode.numChilds < maxCapGlobal) {
			// Find index of previously existing child
			std::cout << "INTERNAL INS NO SPLIT" << std::endl;

			PageHandler childPage1 = this->rTreeFile.PageAt(child1);
			PageHandler childPage2 = this->rTreeFile.PageAt(child2);

			genericNode childNode1(childPage1);
			genericNode childNode2(childPage2);

			int foundIndex = selectedNode.findChild(prevChildId);

			selectedNode.replaceChild(foundIndex, childPage1);
			selectedNode.insertNode(childPage2);
			
			*(childNode1.parentId) = pageId;
			*(childNode2.parentId) = pageId;
			
			this->rTreeFile.MarkDirty(child1);
			this->rTreeFile.UnpinPage(child1);
			this->rTreeFile.FlushPage(child1);

			this->rTreeFile.MarkDirty(child2);
			this->rTreeFile.UnpinPage(child2);
			this->rTreeFile.FlushPage(child2);

			if (!isRoot) propagateUp(*selectedNode.parentId, 0, selectedNode.selfId, -1, selectedNode.mbr, -1);

			this->rTreeFile.MarkDirty(pageId);
			this->rTreeFile.UnpinPage(pageId);
			this->rTreeFile.FlushPage(pageId);

		} else {
			std::cout << "INTERNAL INS SPLITTING" << std::endl;

			int foundIndex = selectedNode.findChild(prevChildId);

			// pick seed
			int bestChildId1, bestChildId2;
			long long minGroupingInefficiency = std::numeric_limits<long long>::max();
			for (int i=0; i < maxCapGlobal; i++) {
				int seedChildPageId1;
				if (i == foundIndex) {
					seedChildPageId1 = child1;
				} else {
					seedChildPageId1 = *(selectedNode.childIds+i);
				}

				PageHandler seedChildPage1 = this->rTreeFile.PageAt(seedChildPageId1);
				genericNode seedChild1(seedChildPage1);
				for (int j=i+1; j < maxCapGlobal+1; j++) {
					int requiredPage;
					if (j==maxCapGlobal) {
						requiredPage = child2;
					} else if (j == foundIndex) {
						requiredPage = child1;
					} else {
						requiredPage = *(selectedNode.childIds+j);
					}

					this->rTreeFileManager.PrintBuffer();

					PageHandler seedChildPage2 = this->rTreeFile.PageAt(requiredPage);
					genericNode seedChild2(seedChildPage2);

					long long currentGroupInnefficiency = groupingInefficiency(seedChild1.mbr, seedChild2.mbr);
					if (minGroupingInefficiency > currentGroupInnefficiency) {
						minGroupingInefficiency = currentGroupInnefficiency;
						bestChildId1 = seedChild1.selfId;
						bestChildId2 = seedChild2.selfId;
					}

					this->unpinAndFlush(seedChild2.selfId);
				}

				this->unpinAndFlush(seedChild1.selfId);

			}

			// Assign others
			PageHandler nodePage1 = this->rTreeFile.NewPage();
			internalNode splittedNode1(nodePage1);
			splittedNode1.initPageData(nodePage1);

			PageHandler bestChildPage1 = this->rTreeFile.PageAt(bestChildId1);
			genericNode bestChild1(bestChildPage1);

			splittedNode1.insertNode(bestChildPage1);
			*(bestChild1.parentId) = splittedNode1.selfId;

			this->rTreeFile.MarkDirty(bestChildId1);
			this->unpinAndFlush(bestChildId1);

			PageHandler nodePage2 = this->rTreeFile.NewPage();
			internalNode splittedNode2(nodePage2);
			splittedNode2.initPageData(nodePage2);

			PageHandler bestChildPage2 = this->rTreeFile.PageAt(bestChildId2);
			genericNode bestChild2(bestChildPage2);

			splittedNode2.insertNode(bestChildPage2);
			*(bestChild2.parentId) = splittedNode2.selfId;

			this->rTreeFile.MarkDirty(bestChildId2);
			this->unpinAndFlush(bestChildId2);

			std::cout << "SPLIT INTERNAL NODES " << splittedNode1.selfId << " " << splittedNode2.selfId << std::endl;

			for (int i=0; i < maxCapGlobal+1; i++) {
				int requiredPage;
				if (i==maxCapGlobal) {
					requiredPage = child2;
				} else if (i == foundIndex) {
					requiredPage = child1;
				} else {
					requiredPage = *(selectedNode.childIds + i);
				}

				if ((requiredPage != bestChildId1) && (requiredPage != bestChildId2)) {

					PageHandler candidateNodePage = this->rTreeFile.PageAt(requiredPage);
					genericNode candidateNode(candidateNodePage);

					if (*splittedNode1.numChilds == maxCapGlobal) {
						splittedNode2.insertNode(candidateNodePage);
						*candidateNode.parentId = splittedNode2.selfId;
						continue;
					}
					if (*splittedNode2.numChilds == maxCapGlobal) {
						splittedNode1.insertNode(candidateNodePage);
						*candidateNode.parentId = splittedNode1.selfId;
						continue;
					}

					long long groupIneff1, groupIneff2, candidateArea;

					candidateArea = area(candidateNode.mbr);
					groupIneff1 = groupingInefficiency(splittedNode1.mbr, candidateNode.mbr) + candidateArea;
					groupIneff2 = groupingInefficiency(splittedNode2.mbr, candidateNode.mbr) + candidateArea;

					if (groupIneff1 < groupIneff2) {
						splittedNode1.insertNode(candidateNodePage);
						*candidateNode.parentId = splittedNode1.selfId;
					} else if (groupIneff1 == groupIneff2) {
						if (area(splittedNode1.mbr) < area(splittedNode2.mbr)) {
							splittedNode1.insertNode(candidateNodePage);
							*candidateNode.parentId = splittedNode1.selfId;
						} else {
							splittedNode2.insertNode(candidateNodePage);
							*candidateNode.parentId = splittedNode2.selfId;
						}
					} else {
						splittedNode2.insertNode(candidateNodePage);
						*candidateNode.parentId = splittedNode2.selfId;
					}

					this->rTreeFile.MarkDirty(requiredPage);
					this->unpinAndFlush(requiredPage);
				}
			}

			this->rTreeFile.DisposePage(pageId);
			this->rTreeFile.FlushPage(pageId);

			if (!isRoot) {
				int newPageId = *selectedNode.parentId;
				int newChild1 = splittedNode1.selfId;
				int newChild2 = splittedNode2.selfId;

				this->rTreeFile.MarkDirty(splittedNode1.selfId);
				this->unpinAndFlush(splittedNode1.selfId);

				this->rTreeFile.MarkDirty(splittedNode2.selfId);
				this->unpinAndFlush(splittedNode1.selfId);

				propagateUp(newPageId, 1, newChild1, newChild2, nullptr, pageId);
			} else {

				PageHandler newRootPage = this->rTreeFile.NewPage();
				internalNode newRoot(newRootPage);
				newRoot.initPageData(newRootPage);

				newRoot.insertNode(nodePage1);
				newRoot.insertNode(nodePage2);

				*(splittedNode1.parentId) = newRoot.selfId;
				*(splittedNode2.parentId) = newRoot.selfId;

				this->rTreeFile.MarkDirty(splittedNode1.selfId);
				this->unpinAndFlush(splittedNode1.selfId);

				this->rTreeFile.MarkDirty(splittedNode2.selfId);
				this->unpinAndFlush(splittedNode1.selfId);

				this->rTreeFile.MarkDirty(newRoot.selfId);
				this->unpinAndFlush(newRoot.selfId);

				*this->rootId = newRoot.selfId;
				this->rTreeFile.MarkDirty(0);

				return;
			}
		}
	} else {
		// Update childs MBR saved in parents node
		std::cout << "JUST UPDATING UPWARD" << std::endl;
		int foundIndex = selectedNode.findChild(child1);

		memcpy(selectedNode.childMBRs[0] + (foundIndex*dimensionalityGlobal), childsUpdatedMBR[0], sizeof(int)*dimensionalityGlobal);
		memcpy(selectedNode.childMBRs[1] + (foundIndex*dimensionalityGlobal), childsUpdatedMBR[1], sizeof(int)*dimensionalityGlobal);

		// Update selfs mbr
		selectedNode.updateMBR(childsUpdatedMBR);

		if (!isRoot) propagateUp(*selectedNode.parentId, 0, selectedNode.selfId, -1, selectedNode.mbr, -1);

		this->rTreeFile.MarkDirty(pageId);
		this->unpinAndFlush(pageId);
	}
}

int rTree::selectLeaf(int pageId, int * point) {
	// std::cout << "---" << std::endl;
	// this->rTreeFileManager.PrintBuffer();
	// std::cout << "---" << std::endl;


	PageHandler currentNodePage = this->rTreeFile.PageAt(pageId);
	NodeType nodeIs = TypeOf(currentNodePage);

	if (nodeIs == leaf) {
		this->unpinAndFlush(pageId);
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

	this->unpinAndFlush(pageId);
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
	this->unpinAndFlush(pageId);
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

	// try
	// {
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
			std::cout << "---" << std::endl;
			tree.rTreeFileManager.PrintBuffer();
			std::cout << "---" << std::endl;
		}
	// }
	// catch(const std::exception& e)
	// {
	// 	std::cerr << e.what() << '\n';
	// }

	queryFile.close();
	outputFile.close();

	tree.rTreeFileManager.ClearBuffer();
	tree.rTreeFileManager.DestroyFile("tree.txt");
};
