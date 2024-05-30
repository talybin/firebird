# vim: noexpandtab tabstop=4

INC_DIR := include
HDR_FILES := $(wildcard ${INC_DIR}/*.hpp)

TEST_DIR := tests

all: single ${TEST_DIR} fbtest
.PHONY: all

single: ${HDR_FILES} make_single.py
	python3 make_single.py

tests::
	cd ${TEST_DIR} && $(MAKE)

fbtest: ${HDR_FILES} fbtest.cpp
	$(CXX) -I${INC_DIR} $@.cpp -o $@ -lfbclient

clean:
	$(RM) -r single fbtest a.out
	cd ${TEST_DIR} && $(MAKE) clean

