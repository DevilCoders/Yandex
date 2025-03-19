PKGNAME         = yandex-jkp-mongo-backup
DATADIR         = /usr/share
YCONFDIR        = $(DESTDIR)$(DATADIR)/yandex-configs/$(PKGNAME)

clean:
	@rm -rf debian/tmp

test:
	true

install:
	install -d $(YCONFDIR)
	#cp -Rp src/* $(YCONFDIR)/
	cp -Rp src/distributed-flock-mongo-backup* $(YCONFDIR)/
	mkdir -p $(DESTDIR)/etc/
	cp -Rp src/mongo-backup.conf $(DESTDIR)/etc/

.PHONY: clean idl
