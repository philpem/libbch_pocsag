.PHONY: all clean test FORCE

UNITYDIR = ./unity

# Set this to .exe on Windows
EXE =

CFLAGS   += -I${UNITYDIR}/src
CXXFLAGS += -I${UNITYDIR}/src
LDFLAGS  +=

UNITY_O = ${UNITYDIR}/src/unity.o

BCHCHAL_O = bchchallenge.o
BCHCHAL_TR_O = bchchallengeRunner.o


# Build everything
all:

# Clean the build directory
clean:
	rm -f ${BCHCHAL_O} ${UNITY_O}

TESTS       := $(basename $(wildcard tests/*.cpp))
#TESTRUNNERS := $(addsuffix _Runner${EXE},${TESTS})
TESTRESULTS := $(addsuffix .testresults,${TESTS})


# Run all the tests
test:	${TESTRESULTS}
	ruby ${UNITYDIR}/auto/stylize_as_junit.rb -r tests

# Run a test and produce the test results
tests/%.testresults:	tests/%_Runner
	$< | tee $@

# Build the Test Runner from the test source and runner source
tests/%_Runner:	tests/%_Runner.cpp tests/%.cpp ${UNITY_O}
	g++ ${CFLAGS} ${LDFLAGS} -o $@ $^

# Build the Test Runner source from the test
# The FORCE rule 
tests/%_Runner.cpp: tests/%.cpp FORCE
	ruby ${UNITYDIR}/auto/generate_test_runner.rb $< $@


# see https://www.gnu.org/software/make/manual/html_node/Force-Targets.html
FORCE:

