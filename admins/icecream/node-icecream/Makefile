GULPFILE := src/gulpfile.js
.PHONY := upload clean build watch debbuild

upload:
	dupload --to yandex-xenial ../

debuild:
	dpkg-buildpackage -b

build: debuild clean

watch:
	./node_modules/.bin/gulp --gulpfile $(GULPFILE) watch

clean:
	debclean

install:
	git submodule update --init --recursive
	npm install
