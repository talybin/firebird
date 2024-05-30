# vim: noexpandtab tabstop=4

INC_DIR := ../include
HDR_FILES := $(wildcard ${INC_DIR}/*.hpp)

SRCS := $(wildcard example_*.cpp)
NAMES := $(SRCS:.cpp=)

all: ${NAMES}
.PHONY: all

example_%: example_%.cpp
	$(CXX) -I${INC_DIR} $< -o $@ -lfbclient

clean:
	$(RM) ${NAMES}

