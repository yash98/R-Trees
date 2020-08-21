flags = -std=c++11 -g
sampleobjects = buffer_manager.o file_manager.o sample_run.o
treeobjects = buffer_manager.o file_manager.o nodes.o rTree.o

rtree: $(treeobjects)
	rm -f tree.txt
	g++ $(flags) -o rtree $(treeobjects)

sample_run : $(sampleobjects)
	g++ $(flags) -o sample_run $(sampleobjects)

%.o: %.cpp
	g++ $(flags) -o $@ -c $^

clean :
	rm -f *.o
	rm -f sample_run
	rm -f rtree
