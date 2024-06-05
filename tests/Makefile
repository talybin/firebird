# vim: noexpandtab tabstop=4

INC_DIR := ../include
HDR_FILES := $(wildcard $(INC_DIR)/*.hpp)

CPPFLAGS := -I$(INC_DIR)
LIBS := -lfbclient

SRCS := $(wildcard test_*.cpp)
TARGETS := $(SRCS:.cpp=)

all: $(TARGETS)

$(TARGETS): $(SRCS) $(HDR_FILES)
	$(CXX) $(CPPFLAGS) $@.cpp -o $@ $(LIBS)

clean::
	$(RM) $(TARGETS)

