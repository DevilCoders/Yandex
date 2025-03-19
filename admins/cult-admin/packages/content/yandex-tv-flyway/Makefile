PKGNAME         = yandex-tv-flyway
DATADIR         = $(DESTDIR)/opt/flywaydb
BINDIR		= $(DESTDIR)/usr/bin

FWSRC		= flyway
FWFILE		= flyway-commandline-3.2.1-linux-x64.tar.gz
FWURL		= https://bintray.com/artifact/download/business/maven/$(FWFILE)

clean:
	@rm -rf debian/tmp
	@rm -rf $(FWSRC) $(FWFILE)

test:
	true

download:
	wget $(FWURL)
	install -d $(FWSRC)
	tar -xf $(FWFILE) -C $(FWSRC) --strip-components=1

install: download
	install -d $(DATADIR) $(BINDIR)
	cp -Rp $(FWSRC)/* $(DATADIR)/
	cp -Rp flyway-wrapper.sh $(BINDIR)

.PHONY: clean idl
