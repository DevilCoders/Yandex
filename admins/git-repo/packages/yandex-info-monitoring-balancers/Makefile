DESTDIR=/usr/local

install:
	install -d -m 755 $(DESTDIR)/usr/local/sbin/
	install -m 755 script/get_uniq_balancers.sh $(DESTDIR)/usr/local/sbin/get_uniq_balancers.sh
	install -m 755 script/slb_check.sh $(DESTDIR)/usr/local/sbin/slb_check.sh
	install -d -m 755 $(DESTDIR)/etc/cron.d/
	install -m 644 cron/yandex-info-monitoring-balancers $(DESTDIR)/etc/cron.d/yandex-info-monitoring-balancers

