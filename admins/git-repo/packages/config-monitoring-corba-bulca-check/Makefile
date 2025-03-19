PKGNAME         = config-monitoring-bulca
DATADIR         = /usr/lib
YCONFDIR        = $(DESTDIR)$(DATADIR)/$(PKGNAME)

test:
	true

install:
	install -d $(YCONFDIR)
	install -m755 src/bulca.sh $(YCONFDIR)/
	install -m644 src/bulca.conf $(DESTDIR)/etc/monrun/conf.d/
