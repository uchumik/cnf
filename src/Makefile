CXX=g++
DEF= -W -Wall -g -O3
INC= -I/opt/local/include -I. -I../lib
LIB= -L/opt/local/lib -L../lib

#bin= cnftagger cnflearn sequencetest
bin= tagger learner libcnf

all: $(bin)

libcnf: cnftagger.o semicnftagger.o cnflearn.o semicnflearn.o sparsevect.o sequence.o dic.o allocmd.o allocpl.o
	$(CXX) $(DEF) -shared -o libcnf.so.0.0.1 \
   cnftagger.o semicnftagger.o cnflearn.o semicnflearn.o sequence.o sparsevect.o dic.o allocmd.o allocpl.o $(INC) $(LIB) -lc

tagger: tagger_main.o cnftagger.o semicnftagger.o sparsevect.o sequence.o dic.o allocmd.o allocpl.o
	$(CXX) $(DEF) tagger_main.o cnftagger.o semicnftagger.o sequence.o sparsevect.o dic.o allocmd.o allocpl.o -o tagger $(INC) $(LIB)

learner: learn_main.o cnflearn.o semicnflearn.o sparsevect.o sequence.o dic.o allocmd.o allocpl.o
	$(CXX) $(DEF) learn_main.o cnflearn.o semicnflearn.o sequence.o sparsevect.o dic.o allocmd.o allocpl.o -o learner $(INC) $(LIB)

tagger_main.o: tagger_main.cpp
	$(CXX) $(DEF) -c tagger_main.cpp $(INC) $(LIB)

cnftagger.o: cnftagger.cpp
	$(CXX) $(DEF) -fPIC -c cnftagger.cpp $(INC) $(LIB)

learn_main.o: learn_main.cpp
	$(CXX) $(DEF) -c learn_main.cpp $(INC) $(LIB)

cnflearn.o: cnflearn.cpp
	$(CXX) $(DEF) -fPIC -c cnflearn.cpp $(INC) $(LIB)

#semicnftagger: semicnftagger_main.o semicnftagger.o sparsevect.o sequence.o dic.o allocmd.o allocpl.o
#	$(CXX) $(DEF) semicnftagger_main.o semicnftagger.o sequence.o sparsevect.o dic.o allocmd.o allocpl.o -o semicnftagger $(INC) $(LIB)

#semicnflearn: semicnflearn_main.o semicnflearn.o sparsevect.o sequence.o dic.o allocmd.o allocpl.o
#	$(CXX) $(DEF) semicnflearn_main.o semicnflearn.o sequence.o sparsevect.o dic.o allocmd.o allocpl.o -o semicnflearn $(INC) $(LIB)


semicnftagger.o: semicnftagger.cpp
	$(CXX) $(DEF) -fPIC -c semicnftagger.cpp $(INC) $(LIB)

semicnflearn.o: semicnflearn.cpp
	$(CXX) $(DEF) -fPIC -c semicnflearn.cpp $(INC) $(LIB)

sparsevect.o: ../lib/sparsevect.cpp
	$(CXX) $(DEF) -fPIC -c ../lib/sparsevect.cpp $(INC) $(LIB)

dictest.o: ../lib/dictest.cpp
	$(CXX) $(DEF) -fPIC -c ../lib/dictest.cpp $(INC) $(LIB)

dic.o: ../lib/dic.cpp
	$(CXX) $(DEF) -fPIC -c ../lib/dic.cpp $(INC) $(LIB)

#sequencetest: sequencetest.o sequence.o allocmd.o
#	$(CXX) $(DEF) sequencetest.o sequence.o allocmd.o\
#	 -o sequencetest $(INC) $(LIB) -lz

#sequencetest.o: ../lib/sequencetest.cpp
#	$(CXX) $(DEF) -c ../lib/sequencetest.cpp $(INC) $(LIB)

sequence.o: ../lib/sequence.cpp
	$(CXX) $(DEF) -fPIC -c ../lib/sequence.cpp $(INC) $(LIB)

allocmd.o: ../lib/allocmd.cpp
	$(CXX) $(DEF) -fPIC -c ../lib/allocmd.cpp $(INC) $(LIB)

allocpl.o: ../lib/allocpl.cpp
	$(CXX) $(DEF) -fPIC -c ../lib/allocpl.cpp $(INC) $(LIB)

clean:
	@rm -rf *.so *.o ../lib/*.o sequencetest cnflearn cnftagger *.core $(bin)
