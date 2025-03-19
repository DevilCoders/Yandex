PKGNAME         = config-nginx-kassa-tickets
DATADIR         = /usr/share
YCONFDIR        = $(DESTDIR)$(DATADIR)/yandex-configs/$(PKGNAME)

clean:
	@rm -rf debian/tmp

test:
	true

install:
	install -d $(DESTDIR)/etc
	cp -Rp src/* $(DESTDIR)/etc/

.PHONY: clean idl
