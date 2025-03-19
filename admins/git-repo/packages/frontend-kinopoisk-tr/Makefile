clean:
	@rm -rf debian/tmp

install:
	install -m644 src/etc/nginx/* $(DESTDIR)/etc/nginx/
	install -m755 src/usr/local/bin/* $(DESTDIR)/usr/local/bin/
	install -m600 src/var/www/.ssh/* $(DESTDIR)/var/www/.ssh/

.PHONY: clean idl
