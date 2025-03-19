clean:
	@rm -rf debian/tmp

test:
	true

install:
	install -d $(DESTDIR)/usr/sbin/
	install -d $(DESTDIR)/etc/cron.d/
	install -d $(DESTDIR)/etc/logrotate.d/
	cp src/media-ipset-ban.pl $(DESTDIR)/usr/sbin/media-ipset-ban.pl
	cp src/media-ipset-ban_cron $(DESTDIR)/etc/cron.d/media-ipset-ban
	cp src/media-ipset-ban_logrotate $(DESTDIR)/etc/logrotate.d/media-ipset-ban

.PHONY: clean idl
