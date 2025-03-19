install:
	install -d $(DESTDIR)/etc
	install -d $(DESTDIR)/etc/init.d
	install -d $(DESTDIR)/etc/sudoers.d
	install -d $(DESTDIR)/etc/logrotate.d
	install -d $(DESTDIR)/etc/cron.d
	install -d $(DESTDIR)/etc/cron.yandex
	install -m755 pgsync.init.d $(DESTDIR)/etc/init.d/pgsync
	install -m400 pgsync.sudoers.d $(DESTDIR)/etc/sudoers.d/pgsync
	install -m644 pgsync.logrotate $(DESTDIR)/etc/logrotate.d/pgsync
	install -m644 wd-pgsync.cron.d $(DESTDIR)/etc/cron.d/wd-pgsync
