.PHONY: all clean test

UNITYDIR = ./unity

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

#
#
#
test: bchchallenge_test
	$(eval TMPD := $(shell mktemp -d -t BCHTest.XXXXXXXX))
	-./bchchallenge_test | tee -a ${TMPD}/bchchallenge.testresults
	-ruby ${UNITYDIR}/auto/stylize_as_junit.rb -r ${TMPD}
	rm -rf ${testfile}


bchchallenge_test:	${BCHCHAL_O} ${BCHCHAL_TR_O} ${UNITY_O}
	g++ ${CFLAGS} ${LDFLAGS} -o $@ $^

bchchallengeRunner.cpp: bchchallenge.cpp
	ruby ${UNITYDIR}/auto/generate_test_runner.rb $^ $@

