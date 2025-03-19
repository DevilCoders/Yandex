DESTDIR=/usr/local

install:
	install -d -m 755 $(DESTDIR)/etc/monrun/conf.d/
	install -m 644 monrun/conf.d/wapconvert.conf $(DESTDIR)/etc/monrun/conf.d/
	install -m 644 monrun/conf.d/ping-wap.conf $(DESTDIR)/etc/monrun/conf.d/
	
	install -m 755 script/pinger.sh $(DESTDIR)/usr/local/sbin/

