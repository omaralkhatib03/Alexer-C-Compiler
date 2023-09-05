CPPFLAGS += -std=c++20 -W -Wall -g -Wno-unused-parameter
CPPFLAGS += -I include

all : bin/c_compiler

src/parser.tab.cpp src/parser.tab.hpp : src/parser.y
	bison -v -d src/parser.y -o src/parser.tab.cpp

src/lexer.yy.cpp : src/lexer.flex src/parser.tab.hpp
	flex -o src/lexer.yy.cpp  src/lexer.flex

bin/c_compiler : src/c_compiler.o src/parser.tab.o src/lexer.yy.o src/parser.tab.o
	mkdir -p bin
	g++ $(CPPFLAGS) -o bin/c_compiler $^

clean :
	rm -rf src/*.o
	rm -rf src/*.tab.cpp
	rm -rf src/*.yy.cpp
	rm -rf src/parser.output
	rm -rf src/parser.tab.hpp
	rm -rf bin/*
	rm -rf custom/complete
	
forceClean:
	sudo rm -rf src/*.o
	sudo rm -rf src/*.tab.cpp
	sudo rm -rf src/*.yy.cpp
	sudo rm -rf src/parser.output
	sudo rm -rf src/parser.tab.hpp
	sudo rm -rf bin/*
	sudo rm -rf custom/complete
