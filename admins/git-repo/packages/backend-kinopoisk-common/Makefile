clean:
	@rm -rf debian/tmp

install:
	install -m644 src/etc/apache2/conf.d/apache2-kp.conf $(DESTDIR)/etc/apache2/conf.d/apache2-kp.conf
	install -m644 src/etc/apache2/sites-available/00-default.conf $(DESTDIR)/etc/apache2/sites-available/00-default.conf
	install -m644 src/etc/apache2/sites-available/01-www.kinopoisk.ru.conf $(DESTDIR)/etc/apache2/sites-available/01-www.kinopoisk.ru.conf
	install -m644 src/etc/apache2/sites-available/02-ext.kinopoisk.ru.conf $(DESTDIR)/etc/apache2/sites-available/02-ext.kinopoisk.ru.conf
	install -m644 src/etc/apache2/sites-available/03-testing.dev.kinopoisk.ru.conf $(DESTDIR)/etc/apache2/sites-available/03-testing.dev.kinopoisk.ru.conf
	install -m644 src/etc/apache2/sites-available/04-prestable.dev.kinopoisk.ru.conf $(DESTDIR)/etc/apache2/sites-available/04-prestable.dev.kinopoisk.ru.conf
	install -m644 src/etc/apache2/sites-available/05-ext-testing.kinopoisk.ru.conf $(DESTDIR)/etc/apache2/sites-available/05-ext-testing.kinopoisk.ru.conf
	install -m644 src/etc/apache2/mime.types $(DESTDIR)/etc/apache2/mime.types
	install -m644 src/etc/tinyproxy-kp.conf $(DESTDIR)/etc/tinyproxy-kp.conf
	install -m644 src/etc/php5/conf.d/.keep $(DESTDIR)/etc/php5/conf.d/.keep
	install -m644 src/etc/php5/php.ini.production $(DESTDIR)/etc/php5/php.ini.production
	install -m644 src/etc/php5/php.ini.testing $(DESTDIR)/etc/php5/php.ini.testing
	install -m644 src/etc/php5/php.ini.prestable $(DESTDIR)/etc/php5/php.ini.prestable
	install -m644 src/home/www/default/index.html $(DESTDIR)/home/www/default/index.html
	install -m644 src/etc/cron.d/* $(DESTDIR)/etc/cron.d/
	install -m600 src/var/www/.ssh/* $(DESTDIR)/var/www/.ssh/
	install -m755 src/usr/local/bin/* $(DESTDIR)/usr/local/bin/

.PHONY: clean idl
