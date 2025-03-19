PKGNAME         = config-nginx-weather-www
DATADIR         = /usr/share
YCONFDIR        = $(DESTDIR)$(DATADIR)/yandex-configs/$(PKGNAME)

clean:
	@rm -rf debian/tmp

test:
	true

install:
	install -d $(DESTDIR)/etc/nginx/
	cp -Rp nginx/* $(DESTDIR)/etc/nginx/

.PHONY: clean idl
