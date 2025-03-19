PKGNAME = caching-dns
DATADIR = /usr/share
YCONFDIR = $(DESTDIR)$(DATADIR)/yandex-configs/$(PKGNAME)

all:

clean:
	echo "You shouldn't pass! (c) Gendalf"
	rm -f *stamp

install:
	install -dD $(YCONFDIR)
	cp -rv unbound  $(YCONFDIR)/
	install -m755 config-caching-dns-update-configs $(DESTDIR)/usr/bin/
	install -m644 config-caching-dns.cron $(DESTDIR)/etc/cron.d/config-caching-dns
	install -d $(DESTDIR)/etc/yandex/config-caching-dns/resolv-hooks.d
