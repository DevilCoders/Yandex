PKGNAME         = yandex-content-kino-nginx-ssl-conf
DATADIR         = /usr/share
YCONFDIR        = $(DESTDIR)$(DATADIR)/yandex-configs/$(PKGNAME)

clean:
	@rm -rf debian/tmp

test:
	true

install:
	install -d $(YCONFDIR)
	cp -Rp src/* $(YCONFDIR)/

.PHONY: clean idl
