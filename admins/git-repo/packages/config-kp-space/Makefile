PKGNAME = config-kp-space
DATADIR = /usr/lib
YCONFDIR = $(DESTDIR)$(DATADIR)/yandex-configs/$(PKGNAME)

# vim: noet
test:
	true
install:
# config
	mkdir -p $(YCONFDIR)
	install -m644 src/push-server-default.yaml $(YCONFDIR)/
	install -m644 src/statbox-server-check.conf $(YCONFDIR)/
	install -m755 src/statbox-server-check.sh $(YCONFDIR)/
	mkdir -p $(DESTDIR)/usr/bin/ $(DESTDIR)/etc/cron.d/ $(DESTDIR)/etc/monrun/conf.d/ $(DESTDIR)/etc/logrotate.d/
	install -m755 src/scripts/update_dev_testing.sh $(DESTDIR)/usr/bin/
	install -m755 src/scripts/seclog-distrub.sh $(DESTDIR)/usr/bin/
	install -m755 src/scripts/check-seclog.sh $(DESTDIR)/usr/bin/
	install -m644 src/cron/update-dev-testing $(DESTDIR)/etc/cron.d/
	install -m644 src/cron/backup-dump $(DESTDIR)/etc/cron.d/
	install -m644 src/logrotate.d/update-dev-testing $(DESTDIR)/etc/logrotate.d/
	install -m644 src/cron/seclog-distrub $(DESTDIR)/etc/cron.d/
	install -m644 src/cron/clean-opt-tmp  $(DESTDIR)/etc/cron.d/
	install -m644 src/monrun/seclog-rsync.conf  $(DESTDIR)/etc/monrun/conf.d/
	cp -rp src/seclog $(DESTDIR)/etc/
	chmod 600 $(DESTDIR)/etc/seclog/*  
