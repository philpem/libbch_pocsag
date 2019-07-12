.PHONY: all clean

UNITYDIR = ./unity

CFLAGS   += -I${UNITYDIR}/src
CXXFLAGS += -I${UNITYDIR}/src
LDFLAGS  +=

UNITY_O = ${UNITYDIR}/src/unity.o

BCHCHAL_O = bchchallenge.o

all: bchchallenge

clean:
	rm -f ${BCHCHAL_O} ${UNITY_O}

bchchallenge:	${BCHCHAL_O} ${UNITY_O}
	g++ ${CFLAGS} ${LDFLAGS} -o $@ $^

