DESTDIR=/usr/local

install:
	install -d -m 755 $(DESTDIR)/etc/monrun/conf.d/
	install -m 644 monrun/conf.d/http.conf $(DESTDIR)/etc/monrun/conf.d/
	install -m 755 script/local_smtp.sh $(DESTDIR)/usr/local/sbin/
	install -m 644 monrun/conf.d/local_smtp.conf $(DESTDIR)/etc/monrun/conf.d/
	install -m 755 script/lock_3306.sh $(DESTDIR)/usr/local/sbin/
	install -m 644 monrun/conf.d/lock_3306.conf $(DESTDIR)/etc/monrun/conf.d/
	install -m 755 script/mysql_ping.sh $(DESTDIR)/usr/local/sbin/
	install -m 644 monrun/conf.d/mysql_ping.conf $(DESTDIR)/etc/monrun/conf.d/
	install -m 644 monrun/conf.d/rsync.conf $(DESTDIR)/etc/monrun/conf.d/
	install -m 644 monrun/conf.d/certs.conf $(DESTDIR)/etc/monrun/conf.d/
