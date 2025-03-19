DESTDIR=/usr/local

install:
	install -d -m 755 $(DESTDIR)/usr/local/sbin/
	install -d -m 755 $(DESTDIR)/etc/monrun/conf.d/
	install -m 755 smailik-prepare-check.sh $(DESTDIR)/usr/local/sbin/smailik-prepare-check.sh
	install -m 644 smailik-prepare-check.conf $(DESTDIR)/etc/monrun/conf.d/smailik-prepare-check.conf

