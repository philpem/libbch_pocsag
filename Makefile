.PHONY: all clean test FORCE

# Set this to .exe on Windows
EXE =


# C/C++ and linker flags
CFLAGS   += -I. -I..
CXXFLAGS += -I. -I..
LDFLAGS  +=

CORE_O   = bch.o


# TODO: Move BCH code into a library or object file
# TODO: add build rules for the BCH code


# Build everything
all:

# Clean the build directory
clean:	testclean






#############################################################################
#
# UNIT TESTS
#
#############################################################################

TESTS       := $(basename $(wildcard tests/*.c) $(wildcard tests/*.cpp))
TESTRUNNERS := $(addsuffix _Runner${EXE},${TESTS})
TESTRESULTS := $(addsuffix .testresults,${TESTS})

# Location of Unity and its compiled object file
UNITYDIR = ./unity
UNITY_O = ${UNITYDIR}/src/unity.o

# Add Unity to the C/C++ build path
CFLAGS   += -I${UNITYDIR}/src
CXXFLAGS += -I${UNITYDIR}/src


# Run all the tests
test:	${TESTRESULTS}
	ruby ${UNITYDIR}/auto/stylize_as_junit.rb -r tests


# Clean up all the tests
testclean:
	-rm -f ${UNITY_O}
	-rm -f ${TESTRUNNERS}
	-rm -f ${TESTRESULTS}
	-rm -f results.xml


# Run a test and produce the test results
tests/%.testresults:	tests/%_Runner
	$< | tee $@

# Build the Test Runner from the test source and runner source
tests/%_Runner:	tests/%_Runner.c tests/%.c ${CORE_O} ${UNITY_O}
	g++ ${CFLAGS} ${LDFLAGS} -o $@ $^

tests/%_Runner:	tests/%_Runner.cpp tests/%.cpp ${CORE_O} ${UNITY_O}
	g++ ${CXXFLAGS} ${LDFLAGS} -o $@ $^


# Build the Test Runner source from the test
tests/%_Runner.cpp: tests/%.cpp FORCE
	ruby ${UNITYDIR}/auto/generate_test_runner.rb  $< $@

tests/%_Runner.c: tests/%.c FORCE
	ruby ${UNITYDIR}/auto/generate_test_runner.rb  $< $@



# Force target -- used to make sure the tests always run
# (more specifically - to always regenerate the test runner)
#   see https://www.gnu.org/software/make/manual/html_node/Force-Targets.html
FORCE:

