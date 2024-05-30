# vim: noexpandtab tabstop=4

INC_DIR := ../include
HDR_FILES := $(wildcard ${INC_DIR}/*.hpp)

SRCS := $(wildcard test_*.cpp)
NAMES := $(SRCS:.cpp=)

all: ${NAMES}
.PHONY: all

test_%: test_%.cpp
	$(CXX) -I${INC_DIR} $< -o $@

clean:
	$(RM) ${NAMES}

