PKGNAME         = config-weather-generators-monitoring
DATADIR         = /usr/share
YCONFDIR        = $(DESTDIR)$(DATADIR)/yandex-configs/$(PKGNAME)

clean:
	@rm -rf debian/tmp

test:
	true

install:
	install -d $(YCONFDIR)
	cp -Rp src/monrun-confs $(YCONFDIR)/
	mkdir -p $(DESTDIR)/usr/bin/ $(DESTDIR)/etc/cron.d/
	cp -Rp src/scripts/* $(DESTDIR)/usr/bin/
	cp -Rp src/cron/check-umbrella-log  $(DESTDIR)/etc/cron.d/

.PHONY: clean idl
