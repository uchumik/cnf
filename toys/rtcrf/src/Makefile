LIB=-L. -L../../../lib
CPPFLAGS=-I. -I../../../lib

sample: main.o featuretemplate.o featuretemplate.tab.o lex.yy.o sequence.o allocmd.o
	g++ -g -O2 -Wall -W main.o featuretemplate.o featuretemplate.tab.o lex.yy.o sequence.o allocmd.o -o sample ${LIB} ${CPPFLAGS}

main.o: featuretemplate.tab.cpp main.cpp
	g++ -g -O2 -c main.cpp ${LIB} ${CPPFLAGS}

sequence.o: ${HOME}/study/cnf/lib/sequence.cpp
	g++ -g -O2 -c ${HOME}/study/cnf/lib/sequence.cpp ${LIB} ${CPPFLAGS}

allocmd.o: ${HOME}/study/cnf/lib/allocmd.cpp
	g++ -g -O2 -c ${HOME}/study/cnf/lib/allocmd.cpp ${LIB} ${CPPFLAGS}

featuretemplate.o: featuretemplate.cpp
	g++ -g -O2 -c featuretemplate.cpp ${LIB} ${CPPFLAGS}

featuretemplate.tab.o: featuretemplate.tab.cpp
	g++ -g -O2 -c featuretemplate.tab.cpp ${LIB} ${CPPFLAGS}

lex.yy.o: lex.yy.cc
	g++ -g -O2 -c lex.yy.cc ${LIB} ${CPPFLAGS}

featuretemplate.tab.cpp: lex.yy.cc
	bison -d -l -p ftmpl_parser_ -o featuretemplate.tab.cpp featuretemplate.y

lex.yy.cc: featuretemplate.l
	flex featuretemplate.l

clean:
	rm -f *.o featuretemplate.tab.* lex.yy.cc sample
