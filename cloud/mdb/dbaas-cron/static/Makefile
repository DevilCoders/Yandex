install:
	install -d $(DESTDIR)/etc
	install -d $(DESTDIR)/etc/init.d
	install -d $(DESTDIR)/etc/logrotate.d
	install -d $(DESTDIR)/etc/dbaas-cron
	install -d $(DESTDIR)/etc/dbaas-cron/modules
	install -d $(DESTDIR)/etc/dbaas-cron/conf.d
	install -m644 dbaas-cron.conf $(DESTDIR)/etc/dbaas-cron/dbaas-cron.conf
	install -m755 dbaas-cron.init.d $(DESTDIR)/etc/init.d/dbaas-cron
	install -m644 dbaas-cron.logrotate $(DESTDIR)/etc/logrotate.d/dbaas-cron
