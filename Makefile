#always compiles when using just make
test/comp: src/main.c src/lex.c src/grammar.c
	cc -Wall -o comp src/main.c src/lex.c src/grammar.c src/tree.c src/lextest.c src/symbolTable.c src/codeGen.c
bin/lexical: src/main.c src/lex.c
	cc -o bin/lexical src/lexmain.c src/lex.c

testtype: comp
	cat test/test8.monga |./comp -noCode
test: comp
	cat test/test1.monga |./comp -noTree
	cat test/test2.monga |./comp -noTree
	cat test/test3.monga |./comp -noTree
	cat test/test4.monga |./comp -noTree
	cat test/test5.monga |./comp -noTree
	cat test/test6.monga |./comp -noTree  
	cat test/test7.monga |./comp -noTree 
	cat test/test8.monga |./comp -noTree 
	cat test/test9.monga |./comp -noTree 
	cat test/examples/program1.monga |./comp -noTree
 

testchecks: comp
	sh test/checks/script.sh
testsyntax: comp
	sh test/syntax/script.sh
testlexical: comp
	sh test/script.sh

testleaks: comp
	@rm -f val.out prog.out
	cat test/leak1.monga | valgrind --track-origins=yes ./comp > prog.out 2> val.out
	cat val.out | grep error
	rm -f val.out prog.out
src/grammar.c: src/grammar.y
	bison -d src/grammar.y
	mv grammar.tab.c src/grammar.c
	mv grammar.tab.h src/grammar.h
src/lex.c: src/rules.lex src/grammar.c
	flex src/rules.lex
	mv lex.yy.c src/lex.c
bin/lexical2:
	cc -o bin/lexical2 src/lex.c -ll
clean: 
	rm -f src/lex.c
	rm -f src/grammar.c
	rm -r bin
	mkdir bin
	rm -r temp
	mkdir temp
	rm -rf comp.dSYM
	rm -f test/*/*.output
	rm -f va.txt
	rm -f val.out
	rm -f grammar.output
	rm -f prog.out
	rm -f *.ll

src/codeGen.o: codeGen.c
	cc -o temp/symbolTable.o -Wall -O2 -c symbolTable.c
src/symbolTable.o: symbolTable.c
	cc -o temp/symbolTable.o -Wall -O2 -c symbolTable.c
src/grammar.o: grammar.c
	cc -o temp/grammar.o -Wall -O2 -c grammar.c
src/tree.o: tree.c
	cc -o temp/tree.o -Wall -O2 -c tree.c
src/main.o:  main.c
	cc -o temp/main.o -Wall -O2 -c main.c
src/lex.o: lex.c
	cc -o temp/lex.o -Wall -O2 -c lex.c
bin/comp: temp/main.o temp/lex.o
	ld -o bin/comp main.o temp/lex.o