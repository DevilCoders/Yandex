PKGNAME = config-monrun-cassandra
SCONFDIR = usr/share/yandex-configs/$(PKGNAME)
YCONFDIR = $(DESTDIR)/usr/share/yandex-configs/$(PKGNAME)


clean:
	@rm -rf debian/tmp
install:
	install -m644 src/etc/monrun/conf.d/* $(DESTDIR)/etc/monrun/conf.d/
	install -m644 src/etc/monitoring/* $(DESTDIR)/etc/monitoring/
	install -m 750 src/usr/bin/*  $(DESTDIR)/usr/bin/

