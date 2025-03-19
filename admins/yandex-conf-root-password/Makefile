build:
	true

install:
	install -m644 $(CURDIR)/src/changepasswd.sh $(DESTDIR)/usr/local/sbin/	
	install -m644 $(CURDIR)/src/hashpass* $(DESTDIR)/usr/share/yandex-configs/yandex-conf-root-password/

clean:
	@rm -rf debian/tmp

.PHONY: clean idl
