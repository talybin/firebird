# vim: noexpandtab tabstop=4
# Enable with set modeline and clean .vim/view/*

INC_DIR := include
HDR_FILES := $(wildcard ${INC_DIR}/*.hpp)

TEST_DIR := tests
EXAMPLE_DIR := examples


all: ${TEST_DIR} ${EXAMPLE_DIR} single fbtest
.PHONY: all

single: ${HDR_FILES} make_single.py
	python3 make_single.py

tests::
	cd ${TEST_DIR} && $(MAKE)

examples::
	cd ${EXAMPLE_DIR} && $(MAKE)

fbtest: ${HDR_FILES} fbtest.cpp
	$(CXX) -I${INC_DIR} $@.cpp -o $@ -lfbclient

debug: ${HDR_FILES} fbtest.cpp
	$(CXX) -ggdb -I${INC_DIR} fbtest.cpp -o fbtest -lfbclient

clean:
	$(RM) -r single fbtest a.out
	cd ${TEST_DIR} && $(MAKE) clean
	cd ${EXAMPLE_DIR} && $(MAKE) clean

