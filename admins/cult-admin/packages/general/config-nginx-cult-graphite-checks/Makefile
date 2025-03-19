clean:
	@rm -rf debian/tmp

test:
	true

install:
	install -m755 src/nginx-rpx.py $(DESTDIR)/usr/lib/yandex-graphite-checks/available/nginx-rpx.py
	install -m755 src/timetail-nginx-log.sh $(DESTDIR)/usr/sbin/timetail-nginx-log.sh
	install -m644 src/configs/nginx-rpx.default.cfg $(DESTDIR)/etc/nginx-cult-graphite-confs/nginx-rpx.default.cfg

.PHONY: clean idl
