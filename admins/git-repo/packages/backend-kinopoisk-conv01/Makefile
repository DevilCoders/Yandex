clean:
	@rm -rf debian/tmp

install:
	install -m644 src/etc/apache2/conf.d/apache2-kp.conf $(DESTDIR)/etc/apache2/conf.d/apache2-kp.conf
	install -m644 src/etc/apache2/sites-enabled/conv01h.kp.yandex.net $(DESTDIR)/etc/apache2/sites-enabled/conv01h.kp.yandex.net
	install -m644 src/etc/apache2/mime.types $(DESTDIR)/etc/apache2/mime.types
	install -m644 src/etc/php5/conf.d/php.ini $(DESTDIR)/etc/php5/conf.d/php.ini
	install -m644 src/etc/cron.d/* $(DESTDIR)/etc/php5/conf.d/
	install -m600 src/var/www/.ssh/* $(DESTDIR)/var/www/.ssh/
	install -m755 src/usr/local/bin/* $(DESTDIR)/usr/local/bin/
	install -m755 src/home/scripts/* $(DESTDIR)/home/scripts/

.PHONY: clean idl
