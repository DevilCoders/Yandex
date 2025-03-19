clean:
	@rm -rf debian/tmp

install:
	install -m644 src/etc/apache2/conf.d/apache2-kp.conf $(DESTDIR)/etc/apache2/conf.d/apache2-kp.conf
	install -m644 src/etc/apache2/sites-enabled/forum.kinopoisk.ru.conf $(DESTDIR)/etc/apache2/sites-enabled/forum.kinopoisk.ru.conf
	install -m644 src/etc/apache2/mime.types $(DESTDIR)/etc/apache2/mime.types
	install -m644 src/etc/php5/conf.d/php.ini $(DESTDIR)/etc/php5/conf.d/php.ini
	install -m644 src/etc/nginx/nginx.conf $(DESTDIR)/etc/nginx/nginx.conf
	cp -rp src/etc/monrun $(DESTDIR)/etc/

.PHONY: clean idl
