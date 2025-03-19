clean:
	@rm -rf debian/tmp

install:
	install -m644 src/etc/nginx/sites-enabled/default $(DESTDIR)/etc/nginx/sites-enabled/default
	install -m644 src/etc/monrun/conf.d/* $(DESTDIR)/etc/monrun/conf.d/
	install -m755 src/usr/lib/config-monitoring-common/* $(DESTDIR)/usr/lib/config-monitoring-common

.PHONY: clean idl
