DESTDIR=/usr/local

install:
	install -d -m 755 $(DESTDIR)/usr/local/sbin/
	install -d -m 755 $(DESTDIR)/etc/monrun/conf.d/
	install -m 755 reminders-mongo-check.sh $(DESTDIR)/usr/local/sbin/reminders-mongo-check.sh
	install -m 644 monrun $(DESTDIR)/etc/monrun/conf.d/reminders-mongo.conf
