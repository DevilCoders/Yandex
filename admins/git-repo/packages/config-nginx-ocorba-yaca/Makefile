DESTDIR=/usr/local

install:
	install -d -m 755 $(DESTDIR)/etc/nginx/sites-available/
	install -m 644 10-yaca.conf $(DESTDIR)/etc/nginx/sites-available/
	install -d -m 755 $(DESTDIR)/etc/monrun/conf.d/
	install -m 644 monitoring $(DESTDIR)/etc/monrun/conf.d/yaca.conf
