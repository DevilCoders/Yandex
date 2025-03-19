SRCROOT := $(patsubst %/,%,$(dir $(lastword $(MAKEFILE_LIST))))

LOG_VERBOSITY := LOG_INFO

INCLUDES = \
	   -I$(SRCROOT)/include \
	   -I$(SRCROOT)
DEFINES = \
	  -D_GNU_SOURCE \
	  -DLOG_VERBOSITY=$(LOG_VERBOSITY)
CPPFLAGS = $(DEFINES) $(INCLUDES)
CFLAGS := \
	 -Wall \
	 -Werror \
	 -Wextra \
	 -Wno-unused-parameter \
	 -pthread \
	 -g -O2
CXXFLAGS = $(CFLAGS)
LDFLAGS = \
	  -pthread

CC = clang
CXX = clang++

compiler-flag = $(shell \
	$(1) -Werror -S -x $(2) $(3) /dev/null -o - >/dev/null 2>&1 && \
	echo "$(3)")
cc-flag = $(call compiler-flag,$(CC),c,$(1))

CFLAGS += \
	$(call cc-flag,-Wmissing-prototypes) \
	$(call cc-flag,-Wmissing-variable-declarations) \
	$(call cc-flag,-Wzero-length-array) \
	$(call cc-flag,-Wzero-length-bounds)

# FIXME: build agents have ancient clang which needs explicit C++ version
CXX += -std=c++11

ifeq ($(WITH_ASAN),1)
CFLAGS += -fsanitize=address
LDFLAGS += -fsanitize=address
endif

ifeq ($(WITH_VALGRIND),1)
TEST_RUNNER = valgrind --leak-check=full --error-exitcode=1 -v
endif

VHD_LIB = $(SRCROOT)/libvhost-server.a

DEPS = $(patsubst %.o,%.d,$(OBJS))
CHECK_RUNS = $(patsubst %,%-check,$(TESTS))
BUILD_SUBDIRS = $(patsubst %,%-build-subdir,$(SUBDIRS))
CHECK_SUBDIRS = $(patsubst %,%-check-subdir,$(SUBDIRS))
CLEAN_SUBDIRS = $(patsubst %,%-clean-subdir,$(SUBDIRS))

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ -c $<
%.d: %.c
	$(CC) $(CPPFLAGS) -o $@ -MM -MP -MQ $@ -MQ $(@:%.d=%.o) -c $<
%.o: %.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ -c $<
%.d: %.cpp
	$(CXX) $(CPPFLAGS) -o $@ -MM -MP -MQ $@ -MQ $(@:%.d=%.o) -c $<
%_test: %_test.o $(VHD_LIB)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $^ $(LDLIBS) -o $@
%_test-check: %_test force-rule
	$(TEST_RUNNER) ./$<
%-build-subdir: force-rule
	$(MAKE) -C $*
%-check-subdir: force-rule
	$(MAKE) -C $* check
%-clean-subdir: force-rule
	$(MAKE) -C $* clean

.PHONY: all check clean force-rule \
	$(BUILD_SUBDIRS) $(CHECK_SUBDIRS) $(CLEAN_SUBDIRS)
