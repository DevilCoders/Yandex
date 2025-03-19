DESTDIR=/usr/local

install:
	install -d -m 755 $(DESTDIR)/etc/haproxy/templates/
	install -m 644 blocks.cfg $(DESTDIR)/etc/haproxy/templates/
	install -m 644 main.cfg $(DESTDIR)/etc/haproxy/templates/
	install -d -m 755 $(DESTDIR)/usr/local/sbin
	install -m 755 monitoring.sh $(DESTDIR)/usr/local/sbin/haproxy-monitoring.sh	
	install -d -m 755 $(DESTDIR)/etc/monrun/conf.d
	install -m 644 monitoring-haproxy.conf $(DESTDIR)/etc/monrun/conf.d/haproxy-monitoring.conf
	install -d -m 755 $(DESTDIR)/etc/syslog-ng/conf.d/
	install -m 644 syslog-ng-haproxy.conf $(DESTDIR)/etc/syslog-ng/conf.d/01haproxy.conf	
