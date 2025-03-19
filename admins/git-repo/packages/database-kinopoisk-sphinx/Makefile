clean:
	@rm -rf debian/tmp

install:
	install -m644 src/etc/sphinxsearch/sphinx.conf $(DESTDIR)/etc/sphinxsearch/sphinx.conf
	install -m755 src/etc/init.d/* $(DESTDIR)/etc/init.d/
	install -m644 src/home/sphinx/* $(DESTDIR)/home/sphinx/
	install -m644 src/etc/init/* $(DESTDIR)/etc/init/
	install -m644 src/usr/lib/mysql/plugin/* $(DESTDIR)/usr/lib/mysql/plugin/
	install -m644 src/etc/default/sphinxsearch $(DESTDIR)/etc/default/sphinxsearch

.PHONY: clean idl
