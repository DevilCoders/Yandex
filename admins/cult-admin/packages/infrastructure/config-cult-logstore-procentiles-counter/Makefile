PKGNAME         = config-cult-logstore-procentiles-counter
DATADIR         = /usr/share
YCONFDIR        = $(DESTDIR)$(DATADIR)/yandex-configs/$(PKGNAME)

clean:
	@rm -rf debian/tmp

test:
	true

install:
	install -m755 src/procentiles-counter.py $(DESTDIR)/usr/local/sbin/procentiles-counter.py
	install -d $(DESTDIR)/etc/procentiles-counter
	install -m644 src/procentiles-counter.cfg $(DESTDIR)/etc/procentiles-counter/procentiles-counter.cfg

.PHONY: clean idl
