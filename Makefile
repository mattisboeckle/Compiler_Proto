all: parser

OBJS = parser.o  \
       codegen.o \
       main.o    \
       tokens.o  \
	   native.o  \

LLVMCONFIG = llvm-config
CPPFLAGS = `$(LLVMCONFIG) --cppflags` -std=c++14
LDFLAGS = `$(LLVMCONFIG) --ldflags` -lpthread -ldl -lz -lncurses -rdynamic
LIBS = `$(LLVMCONFIG) --system-libs --libs core orcjit native`

clean:
	$(RM) -rf parser.cpp parser.hpp parser tokens.cpp $(OBJS)

parser.cpp: parser.y
	bison -d -o $@ $^
	
parser.hpp: parser.cpp

tokens.cpp: tokens.l parser.hpp
	flex -o $@ $^

%.o: %.cpp
	clang++ -c $(CPPFLAGS) -o $@ $<

parser: $(OBJS)
	clang++ -o $@ $(OBJS) $(LIBS) $(LDFLAGS)

proto: parser
	sudo cp parser /usr/local/bin/proto

test: parser test/parser01 test/parser02 test/parser03 test/parser04 test/parser05 test/parser06 test/parser07 test/parser08 test/parser09
		./parser test/parser01
		./parser test/parser02
		./parser test/parser03
		./parser test/parser04
		./parser test/parser05
		./parser test/parser06
		./parser test/parser07
		./parser test/parser08
		./parser test/parser09
