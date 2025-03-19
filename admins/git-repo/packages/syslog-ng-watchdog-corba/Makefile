clean:
	@rm -rf debian/tmp

install:
	install -m644 src/etc/cron.d/syslog-ng-watchdog-corba $(DESTDIR)/etc/cron.d/syslog-ng-watchdog-corba
	install -m644 src/etc/monrun/conf.d/syslog-ng-check-corba.conf $(DESTDIR)/etc/monrun/conf.d/syslog-ng-check-corba.conf

.PHONY: clean idl
