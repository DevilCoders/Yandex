DESTDIR=/usr/local

install:
	install -d -m 755 $(DESTDIR)/usr/local/sbin/
	install -m 755 script/config-push-client-dps-proxy-local.sh $(DESTDIR)/usr/local/sbin/config-push-client-dps-proxy-local.sh
	install -d -m 755 $(DESTDIR)/etc/default/
	install -m 644 conf/default-push-client $(DESTDIR)/etc/default/push-client
	install -d -m 755 $(DESTDIR)/etc/yandex/statbox-push-client/
	install -m 644 conf/config-push-client-dps-proxy-local.yaml $(DESTDIR)/etc/yandex/statbox-push-client/
	install -m 644 conf/config-push-client-dps-proxy-local-aggregate.yaml $(DESTDIR)/etc/yandex/statbox-push-client/
	install -d -m 755 $(DESTDIR)/etc/cron.d/
	install -m 644 cron/config-push-client-dps-proxy-local $(DESTDIR)/etc/cron.d/config-push-client-dps-proxy-local
	install -d -m 755 $(DESTDIR)/etc/monrun/conf.d/
	install -m 644 monrun/push-client.conf $(DESTDIR)/etc/monrun/conf.d/push-client.conf
