build:
	perl -c src/bin/pkgver.pl

install:
	install -m755 $(CURDIR)/src/bin/*  $(DESTDIR)/usr/bin/
	install -m755 $(CURDIR)/src/pkgver.init.d  $(DESTDIR)/etc/init.d/pkgver
	install -m644 $(CURDIR)/src/pkgver-systemd.service  $(DESTDIR)/lib/systemd/system/
	install -m755 $(CURDIR)/src/sbin/pkgver-systemd  $(DESTDIR)/usr/sbin/
	install -m644 $(CURDIR)/src/monrun_pkgver.conf  $(DESTDIR)/etc/monrun/conf.d/
	install -m644 $(CURDIR)/src/monrun_runlevel.conf  $(DESTDIR)/etc/monrun/conf.d/runlevel.conf
	install -m644 $(CURDIR)/src/pkgver.logrotate $(DESTDIR)/etc/logrotate.d/pkgver

install-rpm:
	install -d $(DESTDIR)/usr/bin/
	install -m755 $(CURDIR)/src/bin/monrun_pkgver.sh  $(DESTDIR)/usr/bin/monrun_pkgver.sh
	install -m755 $(CURDIR)/src/bin/pkgver.pl  $(DESTDIR)/usr/bin/pkgver.pl
	install -m755 $(CURDIR)/src/bin/runlevel_check.sh  $(DESTDIR)/usr/bin/runlevel_check.sh
	install -d $(DESTDIR)/etc/init.d/
	install -m755 $(CURDIR)/src/pkgver.init.d  $(DESTDIR)/etc/init.d/pkgver
	install -d $(DESTDIR)/etc/logrotate.d/
	install -m644 $(CURDIR)/src/pkgver.logrotate $(DESTDIR)/etc/logrotate.d/pkgver
	install -d $(DESTDIR)/var/log/pkgver/
	install -d $(DESTDIR)/var/lib/pkgver/
	install -d $(DESTDIR)/etc/monrun/conf.d/
	install -m644 $(CURDIR)/src/monrun_pkgver.conf  $(DESTDIR)/etc/monrun/conf.d/monrun_pkgver.conf
