PWD=`pwd`
MAINTAINER=`gpg -K | grep uid | tail -1 | awk '{print $$2,$$3,$$4}'`
CURRENT_VERSION=`python -c 'import django_tanker; print django_tanker.__version__'`
NEXT_VERSION=`python -c 'import django_tanker; import sys; s = django_tanker.__version__; (major, minor) = map(int, s.split(".")); print ".".join(map(str, [major, minor+1]))'`
DATE=`date "+%a, %d %b %Y %T %z"`
DEB_PACKAGE="python-django-tanker"

nop:
	echo "nop"

release:
	echo -e $(DEB_PACKAGE)" ("$(NEXT_VERSION)") unstable; urgency=low\n" > /tmp/new_tanker_changelog
	git log --no-merges --format="  * [%an] %s" $(CURRENT_VERSION)..HEAD >> /tmp/new_tanker_changelog
	echo -e "" >> /tmp/new_tanker_changelog
	echo -e " -- "$(MAINTAINER)"  "$(DATE)"\n" >> /tmp/new_tanker_changelog
	cat $(PWD)/debian/changelog >> /tmp/new_tanker_changelog
	cat /tmp/new_tanker_changelog > $(PWD)/debian/changelog
	rm /tmp/new_tanker_changelog
	editor $(PWD)/debian/changelog
	debcommit -ra

push_release:
	git push origin master --tags

clean_tests:
	rm -rf $(PWD).tox $(PWD)/tests/fixtures/*.yaml

.PHONY: build test clean_tests
