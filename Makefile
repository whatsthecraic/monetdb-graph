#
# Custom makefile to build the library for the monetdb operator
# Note this only builds the library, you will still need to link monetdb, e.g. :
# ln -s build/libgraph.so <monetdb_build>/lib/monetdb5/lib_graph.so
# ln -s mal/graph.mal <monetdb_build>/lib/monetdb5
# ln -s mal/autoload.mal <monetdb_build>/lib/monetdb5/autoload/50_graph.mal
#

# build path for monetdb
# try to search for the binary
monetdbpath := $(shell if which mserver5 2>/dev/null 1>&2; then dirname $$(dirname $$(which mserver5)); fi)
# otherwise try the default path
ifeq (${monetdbpath},)
monetdbpath := ${HOME}/workspace/monetdb/build/default
monetdbpath := $(shell if [ -d "$(monetdbpath)" ]; then echo "$(monetdbpath)"; fi)
endif
# as above, with a debug build
ifeq (${monetdbpath},)
monetdbpath := ${HOME}/workspace/monetdb/build/debug
monetdbpath := $(shell if [ -d "$(monetdbpath)" ]; then echo "$(monetdbpath)"; fi)
endif
ifeq (${monetdbpath},)
$(error Cannot find the build path of MonetDB)
endif

# Compiler & linker settings
includedir := ${monetdbpath}/include/monetdb/
libdir := ${monetdbpath}/lib/
#common_flags := -O0 -g -Wall -fPIC -I${includedir} -Isrc/
common_flags := -O3 -fPIC -I${includedir} -Isrc/
CFLAGS := ${common_flags}
CXXFLAGS := -std=c++11 ${common_flags}
LDFLAGS := -L${libdir} -lmonetdb5

# In case of clang, emit extra debug info for the C++ objects
ifneq ($(findstring -g, $(CXXFLAGS)),)
CXXNAME := $(shell ${CXX} --version | head -n1 | awk '{print $$1 }')
ifeq (${CXXNAME},clang)
	CXXFLAGS += -fno-limit-debug-info
endif
endif

# Configuration flag, set GRAPHinterjoinlist_SORT to sort the input candidates inside the function
# graph.intersect_join_lists. If unset, the codegen must ensure that the candidates are already sorted
CFLAGS += -DGRAPHinterjoinlist_SORT

# List of the sources to compile
sources := \
	bat_handle.cpp \
	configuration.cpp \
	debug.cpp \
	errorhandling.cpp \
	graph_descriptor.cpp \
	joiner.cpp \
	miscellaneous.c \
	parse_request.cpp \
	prepare.cpp \
	preprocess.c \
	query.cpp \
	spfw.cpp \
	algorithm/sequential/dijkstra/dijkstra.cpp \
	third-party/tinyxml2.cpp

# Name of the produces library
library := libgraph.so

# Build directory
builddir := build

# Helper variables
makedepend_c = @$(CC) -MM $(CPPFLAGS) $(CFLAGS) -MP -MT $@ -MF $(basename $@).d $<
makedepend_cxx = @$(CXX) -MM $(CPPFLAGS) $(CXXFLAGS) -MP -MT $@ -MF $(basename $@).d $<
objectdir := ${builddir}/objects
objectdirs := $(patsubst %./, %, $(sort $(addprefix ${objectdir}/, $(dir ${sources}))))
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
${objects_c} : ${objectdir}/%.o : src/%.c | ${objectdirs}
	${makedepend_c}
	${CC} -c ${CPPFLAGS} ${CFLAGS} $< -o $@

# Objects from C++ files
${objects_cxx} : ${objectdir}/%.o : src/%.cpp | ${objectdirs}
	${makedepend_cxx}
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $< -o $@

# Create the build directory
${builddir} ${objectdirs}:
	mkdir -pv $@

# Dependencies to update the translation units if a header has been altered
-include ${objects:.o=.d}

# Explicit rule, remove the current build
clean:
	rm -rf ${builddir}
