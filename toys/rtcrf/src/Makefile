LIB=-L. -L../../../lib
CPPFLAGS=-I. -I../../../lib

#g++ -W -Wall -g rtcrflearn.cpp rtcrflearn_main.cpp rtcrf.cpp featuretemplate.o featuretemplate.tab.o lex.yy.o sequence.o allocmd.o ../../../lib/dic.cpp ../../../lib/allocpl.cpp ../../../lib/sparsevect.cpp rtcrftagger.cpp -I. -I../../../lib -L.
bin=sample test

all: $(bin)

test: rtcrflearn_main.o rtcrflearn.o rtcrftagger.o rtcrf.o featuretemplate.o featuretemplate.tab.o lex.yy.o sequence.o allocmd.o dic.o allocpl.o sparsevect.o
	g++ -g -O2 -Wall -W rtcrflearn_main.o rtcrflearn.o rtcrftagger.o rtcrf.o featuretemplate.o featuretemplate.tab.o lex.yy.o sequence.o allocmd.o allocpl.o dic.o sparsevect.o -o test ${LIB} ${CPPFLAGS}

rtcrflearn_main.o: rtcrflearn_main.cpp
	g++ -g -O2 -c rtcrflearn_main.cpp ${LIB} ${CPPFLAGS}

rtcrflearn.o : rtcrflearn.cpp
	g++ -g -O2 -c rtcrflearn.cpp ${LIB} ${CPPFLAGS}

rtcrftagger.o : rtcrftagger.cpp
	g++ -g -O2 -c rtcrftagger.cpp ${LIB} ${CPPFLAGS}

rtcrf.o : rtcrf.cpp
	g++ -g -O2 -c rtcrf.cpp ${LIB} ${CPPFLAGS}

sample: main.o featuretemplate.o featuretemplate.tab.o lex.yy.o sequence.o allocmd.o
	g++ -g -O2 -Wall -W main.o featuretemplate.o featuretemplate.tab.o lex.yy.o sequence.o allocmd.o -o sample ${LIB} ${CPPFLAGS}

main.o: featuretemplate.tab.cpp main.cpp
	g++ -g -O2 -c main.cpp ${LIB} ${CPPFLAGS}

sequence.o: ${HOME}/study/cnf/lib/sequence.cpp
	g++ -g -O2 -c ${HOME}/study/cnf/lib/sequence.cpp ${LIB} ${CPPFLAGS}

allocmd.o: ${HOME}/study/cnf/lib/allocmd.cpp
	g++ -g -O2 -c ${HOME}/study/cnf/lib/allocmd.cpp ${LIB} ${CPPFLAGS}

allocpl.o: ${HOME}/study/cnf/lib/allocpl.cpp
	g++ -g -O2 -c ${HOME}/study/cnf/lib/allocpl.cpp ${LIB} ${CPPFLAGS}

dic.o: ${HOME}/study/cnf/lib/dic.cpp
	g++ -g -O2 -c ${HOME}/study/cnf/lib/dic.cpp ${LIB} ${CPPFLAGS}

featuretemplate.o: featuretemplate.cpp
	g++ -g -O2 -c featuretemplate.cpp ${LIB} ${CPPFLAGS}

sparsevect.o: ${HOME}/study/cnf/lib/sparsevect.cpp
	g++ -g -O2 -c ${HOME}/study/cnf/lib/sparsevect.cpp ${LIB} ${CPPFLAGS}

featuretemplate.tab.o: featuretemplate.tab.cpp
	g++ -g -O2 -c featuretemplate.tab.cpp ${LIB} ${CPPFLAGS}

lex.yy.o: lex.yy.cc
	g++ -g -O2 -c lex.yy.cc ${LIB} ${CPPFLAGS}

featuretemplate.tab.cpp: lex.yy.cc
	bison -d -l -p ftmpl_parser_ -o featuretemplate.tab.cpp featuretemplate.y

lex.yy.cc: featuretemplate.l
	flex featuretemplate.l

clean:
	rm -f *.o featuretemplate.tab.* lex.yy.cc sample test
