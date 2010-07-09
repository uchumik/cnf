CXX=g++
DEF= -W -Wall -g -O2
INC= -I/opt/local/include -I. -I../lib
LIB= -L/opt/local/lib -L../lib

#bin= cnftagger cnflearn sequencetest
bin= cnftagger cnflearn semicnftagger semicnflearn

all: $(bin)

cnftagger: cnftagger_main.o cnftagger.o sparsevect.o sequence.o dic.o allocmd.o allocpl.o
	$(CXX) $(DEF) cnftagger_main.o cnftagger.o sequence.o sparsevect.o dic.o allocmd.o allocpl.o -o cnftagger $(INC) $(LIB)

cnflearn: cnflearn_main.o cnflearn.o sparsevect.o sequence.o dic.o allocmd.o allocpl.o
	$(CXX) $(DEF) cnflearn_main.o cnflearn.o sequence.o sparsevect.o dic.o allocmd.o allocpl.o -o cnflearn $(INC) $(LIB)

cnftagger_main.o: cnftagger_main.cpp
	$(CXX) $(DEF) -c cnftagger_main.cpp $(INC) $(LIB)

cnftagger.o: cnftagger.cpp
	$(CXX) $(DEF) -c cnftagger.cpp $(INC) $(LIB)

cnflearn_main.o: cnflearn_main.cpp
	$(CXX) $(DEF) -c cnflearn_main.cpp $(INC) $(LIB)

cnflearn.o: cnflearn.cpp
	$(CXX) $(DEF) -c cnflearn.cpp $(INC) $(LIB)

semicnftagger: semicnftagger_main.o semicnftagger.o sparsevect.o sequence.o dic.o allocmd.o allocpl.o
	$(CXX) $(DEF) semicnftagger_main.o semicnftagger.o sequence.o sparsevect.o dic.o allocmd.o allocpl.o -o semicnftagger $(INC) $(LIB)

semicnflearn: semicnflearn_main.o semicnflearn.o sparsevect.o sequence.o dic.o allocmd.o allocpl.o
	$(CXX) $(DEF) semicnflearn_main.o semicnflearn.o sequence.o sparsevect.o dic.o allocmd.o allocpl.o -o semicnflearn $(INC) $(LIB)

semicnflearn_main.o: semicnflearn_main.cpp
	$(CXX) $(DEF) -c semicnflearn_main.cpp $(INC) $(LIB)

semicnftagger_main.o: semicnftagger_main.cpp
	$(CXX) $(DEF) -c semicnftagger_main.cpp $(INC) $(LIB)

semicnftagger.o: semicnftagger.cpp
	$(CXX) $(DEF) -c semicnftagger.cpp $(INC) $(LIB)

semicnflearn.o: semicnflearn.cpp
	$(CXX) $(DEF) -c semicnflearn.cpp $(INC) $(LIB)

sparsevect.o: ../lib/sparsevect.cpp
	$(CXX) $(DEF) -c ../lib/sparsevect.cpp $(INC) $(LIB)

dictest.o: ../lib/dictest.cpp
	$(CXX) $(DEF) -c ../lib/dictest.cpp $(INC) $(LIB)

dic.o: ../lib/dic.cpp
	$(CXX) $(DEF) -c ../lib/dic.cpp $(INC) $(LIB)

#sequencetest: sequencetest.o sequence.o allocmd.o
#	$(CXX) $(DEF) sequencetest.o sequence.o allocmd.o\
#	 -o sequencetest $(INC) $(LIB) -lz

#sequencetest.o: ../lib/sequencetest.cpp
#	$(CXX) $(DEF) -c ../lib/sequencetest.cpp $(INC) $(LIB)

sequence.o: ../lib/sequence.cpp
	$(CXX) $(DEF) -c ../lib/sequence.cpp $(INC) $(LIB)

allocmd.o: ../lib/allocmd.cpp
	$(CXX) $(DEF) -c ../lib/allocmd.cpp $(INC) $(LIB)

allocpl.o: ../lib/allocpl.cpp
	$(CXX) $(DEF) -c ../lib/allocpl.cpp $(INC) $(LIB)

clean:
	@rm -rf *.o ../lib/*.o sequencetest cnflearn cnftagger *.core $(bin)
