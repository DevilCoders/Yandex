PKGNAME         = yandex-tungsten-autoinstall
DATADIR         = /usr/share
YCONFDIR        = $(DESTDIR)$(DATADIR)/yandex-configs/$(PKGNAME)

clean:
	@rm -rf debian/tmp

test:
	true

install:
	mkdir -p $(DESTDIR)/opt/tungsten-installer/
	cp -Rp src/* $(DESTDIR)/opt/tungsten-installer/

.PHONY: clean idl
