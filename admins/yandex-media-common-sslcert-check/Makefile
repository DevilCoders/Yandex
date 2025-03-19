PKGNAME = yandex-media-common-sslcert-check
BINDIR = $(DESTDIR)/usr/local/bin

clean:
	@rm -rf debian/tmp

test:
	true

install:
	install -d $(BINDIR)
	cp -Rp src/media-common-sslcert-check.sh $(BINDIR)/

.PHONY: clean idl
