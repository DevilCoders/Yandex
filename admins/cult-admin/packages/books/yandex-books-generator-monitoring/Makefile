PKGNAME         = yandex-books-generator-monitoring
DATADIR         = /usr/share
YCONFDIR        = $(DESTDIR)$(DATADIR)/yandex-configs/$(PKGNAME)

clean:
	@rm -rf debian/tmp

test:
	true

install:
	install -d $(YCONFDIR)
	cp -Rp src/* $(YCONFDIR)/
	install -D -m755 src/graphite-checks/books.sh $(DESTDIR)/usr/lib/yandex-graphite-checks/available/books.sh

.PHONY: clean idl
