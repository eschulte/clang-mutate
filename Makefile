CXX := clang++
LLVMCOMPONENTS := cppbackend
RTTIFLAG := -fno-rtti
CXXFLAGS := $(shell llvm-config --cxxflags) $(RTTIFLAG)
LLVMLDFLAGS := $(shell llvm-config --ldflags --libs $(LLVMCOMPONENTS))

SOURCES = clang-mutate.cpp
OBJECTS = $(SOURCES:.cpp=.o)
EXES = $(OBJECTS:.o=)
CLANGLIBS = \
	-lclangFrontend \
	-lclangSerialization \
	-lclangDriver \
	-lclangTooling \
	-lclangParse \
	-lclangSema \
	-lclangAnalysis \
	-lclangEdit \
	-lclangAST \
	-lclangLex \
	-lclangBasic \
	-lclangRewrite \
	-lLLVM-3.2svn

all: $(OBJECTS) $(EXES)
.PHONY: clean check

%: %.o
	$(CXX) -o $@ $< $(CLANGLIBS) $(LLVMLDFLAGS)

clang-mutate: ASTMutate.o clang-mutate.o
	$(CXX) -o $@ $^ $(CLANGLIBS) $(LLVMLDFLAGS)

clean:
	-rm -f $(EXES) $(OBJECTS) ASTMutate.o compile_commands.json a.out hello_* *~

# An alternative to giving compiler info after -- on the command line
compile_commands.json:
	echo -e "[\n\
	  {\n\
	    \"directory\": \"$$(pwd)\",\n\
	    \"command\": \"$$(which clang) $$(pwd)/hello.c\",\n\
	    \"file\": \"$$(pwd)/hello.c\"\n\
	  }\n\
	]\n" > $@

check: clang-mutate hello.c
	./clang-mutate n hello.c -- clang
