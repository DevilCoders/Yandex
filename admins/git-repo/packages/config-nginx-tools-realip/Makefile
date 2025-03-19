DESTDIR=/usr/local

install:
	install -d -m 755 $(DESTDIR)/etc/nginx/sites-enabled/
	install -d -m 755 $(DESTDIR)/etc/nginx/conf.d/
	install -m 644 100-tools-realip.conf $(DESTDIR)/etc/nginx/conf.d/
