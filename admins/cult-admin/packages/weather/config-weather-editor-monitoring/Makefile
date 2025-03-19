PKGNAME         = config-weather-editor-monitoring
DATADIR         = /usr/share
YCONFDIR        = $(DESTDIR)$(DATADIR)/yandex-configs/$(PKGNAME)

clean:
	@rm -rf debian/tmp

test:
	true

install:
	install -d $(YCONFDIR)
	cp -Rp src/monrun-confs $(YCONFDIR)/

.PHONY: clean idl
