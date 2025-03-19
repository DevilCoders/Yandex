PKGNAME         = config-kassa-mail-fallback
DATADIR         = /usr/share
YCONFDIR        = $(DESTDIR)$(DATADIR)/yandex-configs/$(PKGNAME)

clean:
	@rm -rf debian/tmp

test:
	true

install:
	install -d $(DESTDIR)/
	cp -Rp src/* $(DESTDIR)/

.PHONY: clean idl
