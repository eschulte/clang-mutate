CXX := clang++
LLVMCOMPONENTS := cppbackend
RTTIFLAG := -fno-rtti
CXXFLAGS := $(shell llvm-config --cxxflags) $(RTTIFLAG)
LLVMLDFLAGS := $(shell llvm-config --ldflags --libs $(LLVMCOMPONENTS))

SOURCES = mutate-tool.cpp
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

clean:
	-rm -f $(EXES) $(OBJECTS) compile_commands.json a.out hello_* *~

compile_commands.json:
	echo -e "[\n\
	  {\n\
	    \"directory\": \"$$(pwd)\",\n\
	    \"command\": \"$$(which clang) $$(pwd)/hello.c\",\n\
	    \"file\": \"$$(pwd)/hello.c\"\n\
	  }\n\
	]\n" > $@

check: mutate-tool hello.c compile_commands.json
	./mutate-tool n hello.c
