include $(CURDIR)/../../../common.mk

OBJS = \
        buffer.o \
        fuse_log.o \
        fuse_opt.o \
        fuse_signals.o \
        fuse_lowlevel.o \

libfuse.a: $(OBJS)
	$(AR) rcs $@ $?

clean: $(CLEAN_SUBDIRS)
	$(RM) $(DEPS) $(OBJS) libfuse.a

-include $(DEPS)
