DESTDIR=/usr/local

install:
	install -d -m 755 $(DESTDIR)/etc/logmux/
	install -m 644 etc/logmux/collector-deployment.conf $(DESTDIR)/etc/logmux
	install -m 644 etc/logmux/collector-tuning.conf $(DESTDIR)/etc/logmux
	install -d -o logmux -m 755 $(DESTDIR)/var/db/logmux
	chown -R logmux $(DESTDIR)/var/db/logmux
