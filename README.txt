1310844
Lucas Menezes, T6 1715
—————————————————————

Description of monga language:
http://www.inf.puc-rio.br/~roberto/comp/lang.html

First, use make to compile:
make

program reads from standard input and outputs to a.ll, 
then calls clang to generate a binary named a.out
Usage examples:

cat test/helloWorld.monga | ./comp
./a.out

cat test/test6.monga | ./comp
./a.out -c 

Usage Description:  
<string input> | ./comp [options] [clangOptions]

clangOptions list will be redirected to clang

the program accepts some options:

./comp -noBin 
won't call clang to generate a.out
./comp -noTree 
won't output the program tree
./comp -noCode 
won't generate llvm, just output the program tree

to run the tests use,
make testbin

After the message "Syntax OK",
it will start checking types and variables.

If sucessfull, it will output "Types OK" 
and will start printing the tree.

Then, it will generate the llvm output at a.ll.
After that, it will call clang to assemble the
executable: a.out.

./a.out can be executed


To clean builds, use
make clean

To run the tests use
make test

To run the tests for binary compilation only, use
make testbin

To run the tests for the checks only, use
make testchecks

To run the tests for the syntax only, use
make testsyntax

To run the tests for the lexical only, use
make testlexical


Requirements:
C compiler
make
flex
bison
bash
clang/llc (llvm compiler)

rules.lex was developed using flex. 
Although, it was written to also work in lex, it was never tested.

grammar.y was developed using bison.
If bison and yacc differ, bison output should be considered correct for this project


Folder structure:
src/ contains the source code: rules.lex,lex.h and main.c.
lex.c is generated form rules.lex by using flex.
grammar.c & grammar.h are generated using bison

test/ contains test suites. Each folder is a collection of test cases.
For instance, take the checks folder which contains most of the tests
Each test have a name and consist of two files: name.monga and name.answer
name.monga is the input the compiler receives and name.answer, the expected output.
The result and the answer are compared textually. If they are equal, the test is OK. 
If they are different, the difference is shown on console

Each test suite has its own script .

temp/ is a folder for temporary files during a compilation

