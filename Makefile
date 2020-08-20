flags = -std=c++11 -g
sampleobjects = buffer_manager.o file_manager.o sample_run.o
treeobjects = buffer_manager.o file_manager.o nodes.o rTree.o

rtree: 
	g++ $(flags) -o rtree $(treeobjects)

sample_run : $(sampleobjects)
	g++ $(flags) -o sample_run $(sampleobjects)

sample_run.o : sample_run.cpp
	g++ $(flags) -c sample_run.cpp

buffer_manager.o : buffer_manager.cpp
	g++ $(flags) -c buffer_manager.cpp

file_manager.o : file_manager.cpp
	g++ $(flags) -c file_manager.cpp

nodes.o: nodes.cpp
	g++ $(flags) -c nodes.cpp

rTree.o: rTree.cpp
	g++ $(flags) -c rTree.cpp

clean :
	rm -f *.o
	rm -f sample_run
	rm -f rtree
