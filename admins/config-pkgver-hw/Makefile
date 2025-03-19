install:
	#mkdir -p debian/config-pkgver-hw/etc/init.d/
	mkdir -p debian/config-pkgver-hw/usr/sbin/
	mkdir -p debian/config-pkgver-hw/lib/systemd/system/
	#install -m755 $(CURDIR)/src/pkgver-hw.init.d  debian/config-pkgver-hw/etc/init.d/pkgver-hw
	install -m755 $(CURDIR)/src/pkgver-ng  debian/config-pkgver-hw/usr/sbin/pkgver-ng
	install -m644 $(CURDIR)/debian/systemd/* debian/config-pkgver-hw/lib/systemd/system/
