
install:
	install -m644 $(CURDIR)/sbin/autodetect_active_eth $(DESTDIR)/usr/local/sbin/autodetect_active_eth
	install -m644 $(CURDIR)/etc/monrun/conf.d/buggy_nic.conf $(DESTDIR)/etc/monrun/conf.d/buggy_nic.conf
	install -m644 $(CURDIR)/bin/check_broadcom.sh $(DESTDIR)/usr/bin/check_broadcom.sh

install-rpm:
	install -d $(DESTDIR)/usr/local/sbin/
	install -m755 $(CURDIR)/sbin/autodetect_active_eth $(DESTDIR)/usr/local/sbin/autodetect_active_eth
	install -d $(DESTDIR)/etc/monrun/conf.d/
	install -m644 $(CURDIR)/etc/monrun/conf.d/buggy_nic.conf $(DESTDIR)/etc/monrun/conf.d/buggy_nic.conf
	install -d $(DESTDIR)/usr/bin/
	install -m755 $(CURDIR)/bin/check_broadcom.sh $(DESTDIR)/usr/bin/check_broadcom.sh
