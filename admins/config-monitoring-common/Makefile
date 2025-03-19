DESTDIR=/usr/local

install: 
	install -d -m 755 $(DESTDIR)/etc/monrun/conf.d
	install -d -m 755 $(DESTDIR)/etc/monitoring/
	install -d -m 755 $(DESTDIR)/etc/config-monitoring-common/
	install -d -m 755 $(DESTDIR)/usr/lib/config-monitoring-common/
	install -m 644 etc/monrun/conf.d/*.conf $(DESTDIR)/etc/monrun/conf.d/
	install -m 755 usr/lib/config-monitoring-common/*.pl $(DESTDIR)/usr/lib/config-monitoring-common/
	install -m 755 usr/lib/config-monitoring-common/*.sh $(DESTDIR)/usr/lib/config-monitoring-common/
