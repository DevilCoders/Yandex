PKGNAME = check-real-slb
DATADIR = /usr/share
YCONFDIR = $(DESTDIR)$(DATADIR)/yandex-configs/$(PKGNAME)

all:

clean:
	echo "You shouldn't pass! (c) Gendalf"
	rm -f *stamp

install:
	install -m755 sbin/check-real.pl $(DESTDIR)/usr/sbin/check-real
	install -m644 conf/check-real.conf $(DESTDIR)/etc/yandex/check-real/
