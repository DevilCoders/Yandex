PKGNAME         = config-weather-dfront-monitoring
DATADIR         = /usr/share
YCONFDIR        = $(DESTDIR)$(DATADIR)/yandex-configs/$(PKGNAME)

clean:
	@rm -rf debian/tmp

test:
	true

install:
	install -d $(YCONFDIR)
	cp -Rp src/monrun-confs $(YCONFDIR)/
	mkdir -p $(DESTDIR)/usr/bin/
	cp -Rp src/scripts/* $(DESTDIR)/usr/bin/

.PHONY: clean idl
