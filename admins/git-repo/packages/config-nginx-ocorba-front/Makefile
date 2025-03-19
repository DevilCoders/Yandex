DESTDIR=/usr/local

install:
	install -d -m 755 $(DESTDIR)/etc/nginx/sites-available/
	install -m 644 listen_ssl $(DESTDIR)/etc/nginx/
	install -m 644 legal.conf $(DESTDIR)/etc/nginx/sites-available/
	install -m 644 literacy.conf $(DESTDIR)/etc/nginx/sites-available/
	install -m 644 safe.conf $(DESTDIR)/etc/nginx/sites-available/
	install -m 644 visual.conf $(DESTDIR)/etc/nginx/sites-available/
	install -m 644 shell.conf $(DESTDIR)/etc/nginx/sites-available/
	install -m 644 ecir.conf $(DESTDIR)/etc/nginx/sites-available/
	install -m 644 ll.conf $(DESTDIR)/etc/nginx/sites-available/

	install -m 644 default.conf $(DESTDIR)/etc/nginx/sites-available/

	install -d -m 755 $(DESTDIR)/etc/nginx/conf.d/
	install -m 644 access.log.format.conf $(DESTDIR)/etc/nginx/conf.d/
	install -m 644 bucket_size.conf $(DESTDIR)/etc/nginx/conf.d/

	install -d -m 755 $(DESTDIR)/etc/nginx/ssl/
	install -d -m 755 $(DESTDIR)/etc/monrun/conf.d/
	install -m 644 nginx-monitoring $(DESTDIR)/etc/monrun/conf.d/nginx-is-running.conf
	install -d -m 755 $(DESTDIR)/usr/local/sbin/
	install -m 755 nginx-is-running.sh $(DESTDIR)/usr/local/sbin/
	install -d -m 755 $(DESTDIR)/var/www/noc-test/
	install -m 644 ping.html $(DESTDIR)/var/www/noc-test/

	install -d -m 755 $(DESTDIR)/etc/yandex/statbox-push-client/
	install -m 644 push-client.yaml $(DESTDIR)/etc/yandex/statbox-push-client/

	install -d -m 755 $(DESTDIR)/etc/logrotate.d/
	install -m 644 logrotate-nginx.conf $(DESTDIR)/etc/logrotate.d/nginx

