.PHONY: clean all

clean:
	rm -rf ../yamail-monrun-errata_*.build ../yamail-monrun-errata_*.changes ../yamail-monrun-errata_*.deb

install:
	mkdir -p $(DESTDIR)/usr/local/yandex/monitoring
	mkdir -p $(DESTDIR)/etc/monrun/conf.d
	cp scripts/* $(DESTDIR)/usr/local/yandex/monitoring
	chmod 755 $(DESTDIR)/usr/local/yandex/monitoring/*
	cp conf.d/* $(DESTDIR)/etc/monrun/conf.d
	chmod 644 $(DESTDIR)/etc/monrun/conf.d/*

static:
	docker build -t errata-static-build .
	docker run --rm -v $(shell pwd):/dist:rw errata-static-build \
		/usr/local/bin/pyinstaller --upx-dir /usr/bin \
		--log-level=DEBUG --distpath=/dist -s -F \
		/dist/scripts/errata_updates.py
	docker rmi --force errata-static-build
