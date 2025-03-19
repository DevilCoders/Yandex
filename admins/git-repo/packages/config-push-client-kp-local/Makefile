DESTDIR=/usr/local

install:
	install -d -m 755 $(DESTDIR)/usr/local/sbin/
	install -m 755 script/config-push-client-kp-local.sh $(DESTDIR)/usr/local/sbin/config-push-client-kp-local.sh
	install -m 755 script/cache-push-client-kp-local.sh $(DESTDIR)/usr/local/sbin/cache-push-client-kp-local.sh
	install -d -m 755 $(DESTDIR)/etc/default/
	install -m 644 conf/default-push-client $(DESTDIR)/etc/default/push-client
	install -d -m 755 $(DESTDIR)/etc/yandex/statbox-push-client/
	install -m 644 conf/config-push-client-kp-local.yaml $(DESTDIR)/etc/yandex/statbox-push-client/
	install -m 644 conf/config-push-client-kp-local2.yaml $(DESTDIR)/etc/yandex/statbox-push-client/
	install -m 644 conf/config-push-client-kp-local-aggregate.yaml $(DESTDIR)/etc/yandex/statbox-push-client/
	install -m 644 conf/archmaster-push-client.yaml  $(DESTDIR)/etc/yandex/statbox-push-client/
	install -m 644 conf/logbroker-push-client.yaml  $(DESTDIR)/etc/yandex/statbox-push-client/
	install -m 644 conf/logbroker-pre-push-client.yaml  $(DESTDIR)/etc/yandex/statbox-push-client/
	install -m 644 conf/larix-push-client.yaml   $(DESTDIR)/etc/yandex/statbox-push-client/
	install -d -m 755 $(DESTDIR)/etc/cron.d/
	install -m 644 cron/cache-push-client-kp-local $(DESTDIR)/etc/cron.d/cache-push-client-kp-local
	install -m 644 cron/config-push-client-kp-local $(DESTDIR)/etc/cron.d/config-push-client-kp-local
	install -d -m 755 $(DESTDIR)/etc/logrotate.d/
	install -m 644 logrotate.d/* $(DESTDIR)/etc/logrotate.d/
