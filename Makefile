#
# Custom makefile to build the library for the monetdb operator
# Note this only builds the library, you will still need to link monetdb, e.g. :
# ln -s build/libgraph_monetdb.so <monetdb_build>/lib/monetdb5/lib_graph.so
# ln -s mal/graph.mal <monetdb_build>/lib/monetdb5
# ln -s mal/autoload.mal <monetdb_build>/lib/monetdb5/autoload/50_graph.mal
#

# build path for monetdb
hostname := $(shell cat /etc/hostname)
ifeq ($(hostname), sebastian) # home
monetdbpath := ${HOME}/workspace/monetdb/build/
else ifeq ($(hostname), athens.da.cwi.nl) # cwi workstation
monetdbpath := /ufs/dleo/workspace/monetdb/build/
else
$(error Invalid host `$(hostname)')
endif

# Compiler & linker settings
includedir := ${monetdbpath}/include/monetdb/
libdir := ${monetdbpath}/lib/
common_flags := -O0 -g -fPIC -I${includedir}
CFLAGS := ${common_flags}
CXXFLAGS := -std=c++11 ${common_flags}
LDFLAGS := -L${libdir} -lmonetdb5

# List of the sources to compile
sources := preprocess.c spfw.cpp

# Name of the produces library
library := libgraph_monetdb.so

# Build directory
builddir := build

# Helper variables
makedepend_c = @$(CC) -MM $(CPPFLAGS) $(CFLAGS) -MP -MT $@ -MF $(basename $@).d $<
makedepend_cxx = @$(CXX) -MM $(CPPFLAGS) $(CXXFLAGS) -MP -MT $@ -MF $(basename $@).d $<
objectdir := ${builddir}/objects
objects_c := $(addprefix ${objectdir}/, $(patsubst %.c, %.o, $(filter %.c, ${sources})))
objects_cxx := $(addprefix ${objectdir}/, $(patsubst %.cpp, %.o, $(filter %.cpp, ${sources})))
objects := ${objects_c} ${objects_cxx}

.DEFAULT_GOAL = all
.PHONY = all clean

all: ${builddir}/${library}

# Library to build
${builddir}/${library} : ${objects} | ${builddir}
	${CXX} -shared $^ ${LDFLAGS} -o $@

# According to https://www.gnu.org/software/make/manual/html_node/Pattern-Match.html#Pattern-Match
# I would have expected that the rules:
# 1) ${objects} : ${objectdir}/%.o : src/%.c
# 2) ${objects} : ${objectdir}/%.o : src/%.cpp
# would have discriminated C++ and C source files. Instead it coalesces the two rules together, e.g.
# build/objects/preprocess.o: src/preprocess.cpp src/preprocess.c

# Objects from C files
${objects_c} : ${objectdir}/%.o : src/%.c | ${objectdir}
	${makedepend_c}
	${CC} -c ${CPPFLAGS} ${CFLAGS} $< -o $@

# Objects from C++ files
${objects_cxx} : ${objectdir}/%.o : src/%.cpp | ${objectdir}
	${makedepend_cxx}
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $< -o $@

# Create the build directory
${builddir} ${objectdir}:
	mkdir -pv $@

# Dependencies to update the translation units if a header has been altered
-include ${objects:.o=.d}

# Explicit rule, remove the current build
clean:
	rm -rf ${builddir}