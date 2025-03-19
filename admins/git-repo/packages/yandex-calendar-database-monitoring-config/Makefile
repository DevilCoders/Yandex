DESTDIR=/usr/local

install:
	install -d -m 755 $(DESTDIR)/etc/monrun/conf.d/
	install -d -m 755 $(DESTDIR)/usr/local/sbin/
	install -m 644 monrun/conf.d/connection_db.conf $(DESTDIR)/etc/monrun/conf.d/
	install -m 755 script/yandex-calendar-database-monitoring-config.sh $(DESTDIR)/usr/local/sbin/yandex-calendar-database-monitoring-config.sh


