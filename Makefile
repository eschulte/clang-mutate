CXX := clang++
LLVMCOMPONENTS := cppbackend
RTTIFLAG := -fno-rtti
CXXFLAGS := $(shell llvm-config --cxxflags) $(RTTIFLAG)
LLVMLDFLAGS := $(shell llvm-config --ldflags --libs $(LLVMCOMPONENTS))

SOURCES = mutate.cpp
OBJECTS = $(SOURCES:.cpp=.o)
BASE_OBJECTS = ASTMutator.o
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

%: $(BASE_OBJECTS) %.o
	$(CXX) -o $@ $^ $(CLANGLIBS) $(LLVMLDFLAGS)

clean:
	-rm -f $(EXES) $(OBJECTS) $(BASE_OBJECTS) compile_commands.json a.out hello*_* *~

compile_commands.json:
	echo -e "[\n\
	  {\n\
	    \"directory\": \"$$(pwd)\",\n\
	    \"command\": \"$$(which clang) $$(pwd)/hello.c\",\n\
	    \"file\": \"$$(pwd)/hello.c\"\n\
	  }\n\
	]\n" > $@

check: clang-mutate hello.c compile_commands.json
	./clang-mutate n hello.c
