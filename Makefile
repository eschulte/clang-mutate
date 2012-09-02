CXX := clang++
LLVMCOMPONENTS := cppbackend
RTTIFLAG := -fno-rtti
CXXFLAGS := $(shell llvm-config --cxxflags) $(RTTIFLAG)
LLVMLDFLAGS := $(shell llvm-config --ldflags --libs $(LLVMCOMPONENTS))
DDD := $(shell echo $(LLVMLDFLAGS))

SOURCES = mutate.cpp mutate-tool.cpp
OBJECTS = $(SOURCES:.cpp=.o)
EXES = $(OBJECTS:.o=)
CLANGLIBS = \
    -lclangARCMigrate \
    -lclangRewrite \
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
    -lclangBasic
    # -lclangStaticAnalyzerFrontend \
    # -lclangStaticAnalyzerCheckers \
    # -lclangStaticAnalyzerCore \
    # -lclangARCMigrate \

all: $(OBJECTS) $(EXES)

%: %.o
	$(CXX) -o $@ $< $(CLANGLIBS) $(LLVMLDFLAGS) \
		-I/usr/local/src/llvm/include \
		-I/usr/local/src/llvm/tools/clang/include

clean:
	-rm -f $(EXES) $(OBJECTS) hello_* *~
