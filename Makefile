# vim: noexpandtab tabstop=4
# Enable with set modeline and clean .vim/view/*

INC_DIR := include
HDR_FILES := $(wildcard $(INC_DIR)/*.hpp)

CPPFLAGS := -I$(INC_DIR)
LIBS := -lfbclient

TEST_DIR := tests
EXAMPLE_DIR := examples


all: $(TEST_DIR) $(EXAMPLE_DIR) single
.PHONY: all

docs::
	doxygen Doxyfile

single: $(HDR_FILES) make_single.py
	python3 make_single.py

tests::
	cd $(TEST_DIR) && $(MAKE)

examples::
	cd $(EXAMPLE_DIR) && $(MAKE)

clean:
	$(RM) -r single a.out
	cd $(TEST_DIR) && $(MAKE) clean
	cd $(EXAMPLE_DIR) && $(MAKE) clean

