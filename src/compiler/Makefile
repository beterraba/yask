##############################################################################
## YASK: Yet Another Stencil Kernel
## Copyright (c) 2014-2018, Intel Corporation
## 
## Permission is hereby granted, free of charge, to any person obtaining a copy
## of this software and associated documentation files (the "Software"), to
## deal in the Software without restriction, including without limitation the
## rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
## sell copies of the Software, and to permit persons to whom the Software is
## furnished to do so, subject to the following conditions:
## 
## * The above copyright notice and this permission notice shall be included in
##   all copies or substantial portions of the Software.
## 
## THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
## IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
## FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
## AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
## LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
## FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
## IN THE SOFTWARE.
##############################################################################

# Makefile for the YASK stencil compiler.
# Type 'make help' for usage.

# Vars starting with 'YC_' apply to the stencil compiler.

# YASK dirs.
YASK_BASE	:=	$(shell cd ../..; pwd)
LIB_DIR		:=	$(YASK_BASE)/lib
INC_DIR		:=	$(YASK_BASE)/include
BIN_DIR		:=	$(YASK_BASE)/bin
YASK_DIR	:=	$(YASK_BASE)/yask
STENCIL_DIR	:= 	$(YASK_BASE)/src/stencils

# OS-specific
ifeq ($(shell uname -o),Cygwin)
  SO_SUFFIX	:=	.dll
  RUN_PREFIX	:=	env PATH="${PATH}:$(LIB_DIR):$(YASK_DIR)"
  PYTHON	:=	python3
  CXX_PREFIX	:=
else
  SO_SUFFIX	:=	.so
  RUN_PREFIX	:=
  PYTHON	:=	python
  CXX_PREFIX	:=	
endif

# Output files and dirs.
YC_BASE		:=	yask_compiler
YC_MODULE	:=	$(YC_BASE)
YC_EXEC		:=	$(BIN_DIR)/$(YC_BASE).exe
YC_TEST_EXEC	:=	$(BIN_DIR)/$(YC_BASE)_api_test.exe
YC_TEST_EXEC_WITH_EXCEPTION	:=	$(BIN_DIR)/$(YC_BASE)_api_exception_test.exe
YC_LIB		:=	$(LIB_DIR)/lib$(YC_BASE)$(SO_SUFFIX)
YC_PY_LIB	:=	$(YASK_DIR)/_$(YC_MODULE)$(SO_SUFFIX)
YC_PY_MOD	:=	$(YASK_DIR)/$(YC_MODULE).py

# Common source.
COMM_DIR	:=	../common
COMM_SRC_NAMES	:=	output common_utils tuple
COMM_SRC_BASES	:=	$(addprefix $(COMM_DIR)/,$(COMM_SRC_NAMES))

# Compiler source files and dirs.
YC_SWIG_DIR	:=	./swig
YC_LIB_DIR	:=	./lib
YC_SRC_NAMES	:=	Expr ExprUtils Grid Eqs Print Vec Cpp CppIntrin YaskKernel Soln
YC_SRC_BASES	:=	$(addprefix $(YC_LIB_DIR)/,$(YC_SRC_NAMES))
YC_OBJS		:=	$(addsuffix .o,$(YC_SRC_BASES) $(COMM_SRC_BASES))
YC_STENCIL_BASES:=	$(patsubst %.cpp,%,$(wildcard $(STENCIL_DIR)/*.cpp))
YC_STENCIL_OBJS	:=	$(addsuffix .o,$(YC_STENCIL_BASES))
YC_INC_DIRS	:=	$(INC_DIR) $(YC_LIB_DIR) $(COMM_DIR)
YC_INC_GLOB	:=	$(addsuffix /*.hpp,$(YC_INC_DIRS))

# Compiler and default flags.
YC_CXX    	?=	g++  # usually faster than icpc for building the compiler.
YC_CXXOPT	:= 	-O2
YC_CXXFLAGS 	:=	-g -std=c++11 -Wall $(YC_CXXOPT)
YC_CXXFLAGS	+=	$(addprefix -I,$(YC_INC_DIRS) $(STENCIL_DIR))

# Add user-defined flags.
YC_CXXFLAGS	+=	$(EXTRA_YC_CXXFLAGS)

# Linker.
YC_LD		:=	$(YC_CXX)
YC_LFLAGS	:=	-lrt -Wl,-rpath=$(LIB_DIR) -L$(LIB_DIR) -l$(YC_BASE)

# Tools.
SWIG		:=	swig

# Find include path needed for python interface.
# NB: constructing string inside print() to work for python 2 or 3.
PYINC		:= 	$(addprefix -I,$(shell $(PYTHON) -c 'import distutils.sysconfig; print(distutils.sysconfig.get_python_inc() + " " + distutils.sysconfig.get_python_inc(plat_specific=1))'))

RUN_PYTHON	:= 	env PYTHONPATH=$(LIB_DIR):$(YASK_DIR):$(PYTHONPATH) $(PYTHON)

# Compile rules.
# For example stencils,
# - Build at O0 to avoid C++ compiler wasting time optimizing them.
# - Set macro to use internal DSL instead of conflicting API operators.

$(STENCIL_DIR)/%.o: $(STENCIL_DIR)/%.cpp $(YC_INC_GLOB)
	$(CXX_PREFIX) $(YC_CXX) $(YC_CXXFLAGS) -DUSE_INTERNAL_DSL -O0 -fPIC -c -o $@ $<
	@ls -l $@

%.o: %.cpp $(YC_INC_GLOB)
	$(CXX_PREFIX) $(YC_CXX) $(YC_CXXFLAGS) -fPIC -c -o $@ $<
	@ls -l $@

######## Primary targets.

default: compiler

# Compile the stencil compiler.
compiler: $(YC_EXEC)
	@echo "Binary" $(YC_EXEC) "has been built."
	$(MAKE) old-code-check

$(YC_LIB): $(YC_OBJS)
	$(CXX_PREFIX) $(YC_CXX) $(YC_CXXFLAGS) -shared -o $@ $^
	@ls -l $@

$(YC_EXEC): main.cpp $(YC_STENCIL_OBJS) $(YC_LIB)
	$(CXX_PREFIX) $(YC_LD) $(YC_CXXFLAGS) $^ $(YC_LFLAGS) -o $@
	@ls -l $@
	$(MAKE) echo-settings

######## API targets

# Build C++ and Python compiler API libs.
# (Makes sure the compiler exists also, to avoid problems
# when trying to build the kernel w/o a compiler binary.)
api: compiler $(YC_PY_LIB)

# Build python compiler API lib.
# Also builds $(YC_PY_MOD)
$(YC_SWIG_DIR)/yask_compiler_api_wrap.cpp: $(YC_SWIG_DIR)/yask*.i $(INC_DIR)/*.hpp
	$(SWIG) -version
	$(SWIG) -v -DYC_MODULE=$(YC_MODULE) -cppext cpp \
	  -I$(INC_DIR) -I$(COMM_DIR) -I$(COMM_DIR)/swig \
	  -c++ -python -outdir $(YASK_DIR) -builtin $<

# Turn off asserts to work around known SWIG issue:
# https://github.com/swig/swig/issues/773
$(YC_SWIG_DIR)/yask_compiler_api_wrap.o: $(YC_SWIG_DIR)/yask_compiler_api_wrap.cpp
	$(CXX_PREFIX) $(YC_CXX) $(YC_CXXFLAGS) -DNDEBUG $(PYINC) -fPIC -c -o $@ $<
	@ls -l $@

$(YC_PY_LIB): $(YC_OBJS) $(YC_SWIG_DIR)/yask_compiler_api_wrap.o
	$(CXX_PREFIX) $(YC_CXX) $(YC_CXXFLAGS) -shared -o $@ $^
	@ls -l $@

#### API tests.

# Build C++ compiler API test.
$(YC_TEST_EXEC): ./tests/yask_compiler_api_test.cpp $(YC_LIB)
	$(CXX_PREFIX) $(YC_CXX) $(YC_CXXFLAGS) $< $(YC_LFLAGS) -o $@
	@ls -l $@

# Run Python compiler API test to create stencil-code file.
# Also create .pdf rendering of stencil AST if Graphviz is installed.
py-yc-api-test: $(BIN_DIR)/yask_compiler_api_test.py $(YC_PY_LIB)
	@echo '*** Running the Python YASK compiler API test...'
	$(RUN_PYTHON) $<
	- dot -Tpdf -O yc-api-test-py.dot
	ls -l yc-api-test-py.*

# Run C++ compiler API test to create stencil-code file.
# Also create .pdf rendering of stencil AST if Graphviz is installed.
cxx-yc-api-test: $(YC_TEST_EXEC)
	@echo '*** Running the C++ YASK compiler API test...'
	$<
	- dot -Tpdf -O yc-api-test-cxx.dot
	ls -l yc-api-test-cxx.*

# Build C++ compiler API test with exception.
$(YC_TEST_EXEC_WITH_EXCEPTION): ./tests/yask_compiler_api_exception_test.cpp $(YC_LIB)
	$(CXX_PREFIX) $(YC_CXX) $(YC_CXXFLAGS) $< $(YC_LFLAGS) -o $@
	@ls -l $@

# Run Python compiler API test with exceptions to create stencil-code file.
# Also create .pdf rendering of stencil AST if Graphviz is installed.
py-yc-api-test-with-exception: $(BIN_DIR)/yask_compiler_api_exception_test.py $(YC_PY_LIB)
	@echo '*** Running the Python YASK compiler API test with exception...'
	$(RUN_PYTHON) $<

# Run C++ compiler API test with exceptions to create stencil-code file.
# Also create .pdf rendering of stencil AST if Graphviz is installed.
cxx-yc-api-test-with-exception: $(YC_TEST_EXEC_WITH_EXCEPTION)
	@echo '*** Running the C++ YASK compiler API test with exception...'
	$<

######## Misc targets

api-tests:
	$(MAKE) cxx-yc-api-test
	$(MAKE) py-yc-api-test
	$(MAKE) cxx-yc-api-test-with-exception
	$(MAKE) py-yc-api-test-with-exception

all-tests: api-tests

all:
	$(MAKE) compiler
	$(MAKE) api
	$(MAKE) all-tests

old-code-check: $(wildcard $(STENCIL_DIR)/*Stencil.hpp)
	@ for i in $^; do echo "*** WARNING: '$$i' detected. Stencils should now be in .cpp files. If this file is unmodified, just delete it. If modified or new, change its suffix to .cpp."; done

# Remove intermediate files.
clean:
	rm -fv *.s
	rm -fr $(YC_SWIG_DIR)/build
	rm -fv $(YC_SWIG_DIR)/*_api_wrap.*
	rm -fv $(YC_OBJS) $(YC_STENCIL_OBJS)
	- find . -name '*.o' -print -delete
	- find . $(BIN_DIR) $(LIB_DIR) -name '*.optrpt' -print -delete

# Remove executables, libs, etc.
realclean: clean
	rm -fv $(YC_LIB) $(YC_EXEC) $(YC_TEST_EXEC) $(YC_TEST_EXEC_WITH_EXCEPTION) $(YC_PY_MOD)* $(YC_PY_LIB)
	rm -fv *api-test*.dot* *api-test*.hpp*
	- find . -name '*.pyc' -print -delete
	- find . -name '*~' -print -delete

echo-settings:
	@echo "Build environment for" $(YC_EXEC) on `date`
	@echo host=`hostname`
	@echo YC_CXX=$(YC_CXX)
	@$(YC_CXX) --version
	@echo YC_CXXFLAGS="\"$(YC_CXXFLAGS)\""

help:
	@echo "Example YASK-compiler make commands:"
	@echo " $(MAKE) -j compiler"
	@echo " $(MAKE) -j compiler YC_CXXOPT=-O0"
	@echo " $(MAKE) -j api"
	@echo " $(MAKE) -j all-tests"
	@echo " $(MAKE) clean"
	@echo " $(MAKE) realclean"
