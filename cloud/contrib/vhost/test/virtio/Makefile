include $(CURDIR)/../../common.mk

LDLIBS = -lcunit

TEST_OBJS = \
	    virtq_test.o \
	    virtio_blk_test.o

OBJS = \
       $(TEST_OBJS)

TESTS = $(patsubst %.o,%,$(TEST_OBJS))
BINS = $(TESTS)

all: $(TESTS)
check: $(CHECK_RUNS)

clean:
	$(RM) $(DEPS) $(OBJS) $(BINS)

$(VHD_LIB): force-rule
	$(MAKE) -C $(SRCROOT)

-include $(DEPS)
