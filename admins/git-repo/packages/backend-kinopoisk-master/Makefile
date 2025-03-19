clean:
	@rm -rf debian/tmp

install:
	install -m644 src/etc/apache2/conf.d/apache2-kp.conf $(DESTDIR)/etc/apache2/conf.d/apache2-kp.conf
	install -m644 src/etc/apache2/sites-enabled/* $(DESTDIR)/etc/apache2/sites-enabled/
	install -m644 src/etc/apache2/mime.types $(DESTDIR)/etc/apache2/mime.types
	install -m644 src/etc/php5/conf.d/php.ini $(DESTDIR)/etc/php5/conf.d/php.ini
	install -m600 src/var/www/.ssh/* $(DESTDIR)/var/www/.ssh/
	install -m644 src/etc/nginx/nginx.conf $(DESTDIR)/etc/nginx/nginx.conf
	install -m644 src/etc/nginx/ssl_auth/* $(DESTDIR)/etc/nginx/ssl_auth/
	install -m755 src/usr/local/bin/* $(DESTDIR)/usr/local/bin/
	install -m644 src/etc/logrotate.d/nginx $(DESTDIR)/etc/logrotate.d/nginx

.PHONY: clean idl
