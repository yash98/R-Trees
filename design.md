# Design

## Key Decisions
1. Each Node spans a page, even if it doesn't fill the page fully
2. One page will have $max(floor(PAGE_CONTENT_SIZE/(4∗d)),remaining_number)$ number of points. This is done so that one point is in exactly one page, and doesn't span multiple pages.

## Data Structures
1. Parent Nodes
   1. self MBR = 2*d ints
   2. child id = maxCap ints
2. Leaf Nodes
   1. self MBR = 2*d ints
   2. maxCap points (d dimensions) = maxCap * d ints

## Bulk Load
1. Command Format - BULKLOAD filename, number of points
2. Reading points from binary file -  
3. file filename stores the number of points to insert, they are also sorted

### Algo

1. Read binary file to iteratively get points until exhausted
2. Create Leaf nodes each leaf contains min(maxCap, numRemainingPoints) points. It will create ceil(numTotalPoints/maxCap) number of leafs
3. Assign Parent - 
   1. Since bulk load is the very first operation nodes formed by it are on contiguous page numbers. NodesDivide nodes into maxCap sized group and assign this group a common parent forming the node on the next free page.
   2. Since parents are assigned continuously without any other page creation they are also in contiguous page number. They can be passed recursively to Assign parent to assign parents for the current parent nodes
   3. Stop when only 1 parent is created as it is the root 

## Insertion
1. Command Format - INSERT d ints

### Algo
1. Find Leaf
   1. Traverse to the right node by selecting the MBR which leads to minimum area enlargement
   2. If node had space for another child just propagate changes to MBR to its parents
2. Split Node
   1. In case number of children reach maxCap + 1 during insertion, the current node is split into 2
   2. Quadratic Split - pick 2 children (points or nodes (rectangle)) at a time, calculate the pair which will have largest area wasted in the resulting MBR if the 2 children were under 1 MBR
   3. Keep this 2 children under separate nodes and assign the remaining nodes one by one to either of the nodes, select the node by again calculating least area enlargement
3. Propagate
   1. In case of splits the the parent of the children is identified about the split if it can accommodate the new child it does, other wise according to quadratic split
