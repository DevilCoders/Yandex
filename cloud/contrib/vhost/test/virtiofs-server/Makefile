include $(CURDIR)/../../common.mk

OBJS = \
    fuse_virtio.o \
    passthrough_ll.o \
    helper.o \

INCLUDES += \
	   -I$(SRCROOT)/test/virtiofs-server/libfuse

CFLAGS += $(INCLUDES)

LIBFUSE = $(SRCROOT)/test/virtiofs-server/libfuse/libfuse.a

virtiofs-server: $(OBJS) $(VHD_LIB) $(LIBFUSE)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

$(LIBFUSE): force-rule
	$(MAKE) -C libfuse

$(VHD_LIB): force-rule
	$(MAKE) -C $(SRCROOT)

SUBDIRS = \
	  libfuse

clean: $(CLEAN_SUBDIRS)
	$(RM) $(DEPS) $(OBJS) virtiofs-server

-include $(DEPS)
