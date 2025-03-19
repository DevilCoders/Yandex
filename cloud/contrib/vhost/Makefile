include common.mk

OBJS = \
       blockdev.o \
       event.o \
       fs.o \
       logging.o \
       memlog.o \
       memmap.o \
       server.o \
       vdev.o \
       virtio/virt_queue.o \
       virtio/virtio_blk.o \
       virtio/virtio_fs.o

$(VHD_LIB): $(OBJS)
	$(AR) rcs $@ $?

SUBDIRS = \
	  test/virtio \
	  test

check: $(CHECK_SUBDIRS)
# FIXME: compatibility with CI; to be removed once adjusted there
test: check

clean: $(CLEAN_SUBDIRS)
	$(RM) $(DEPS) $(OBJS) $(VHD_LIB)

GEN_FILE_LIST = git grep -lI .
TAGS: force-rule
	$(RM) $@
	$(GEN_FILE_LIST) | xargs etags --append
tags: force-rule
	$(RM) $@
	$(GEN_FILE_LIST) | xargs ctags --append
cscope: force-rule
	$(GEN_FILE_LIST) | cscope -qbi -

-include $(DEPS)
