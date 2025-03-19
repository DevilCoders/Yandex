PKGNAME         = config-kp-haproxy-backend
DATADIR         = /usr/share
YCONFDIR        = $(DESTDIR)$(DATADIR)/yandex-configs/$(PKGNAME)

clean:
	@rm -rf debian/tmp

test:
	true

install:
#	install -d $(YCONFDIR)
	cp -Rp src/* $(DESTDIR) 

.PHONY: clean idl
