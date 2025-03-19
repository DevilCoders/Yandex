DESTDIR=/usr/local/

default:
	perl -c $(CURDIR)/timetail
	perl -c $(CURDIR)/timecut
	perl -c $(CURDIR)/timesplit

install:
	install -d $(DESTDIR)/usr/bin
	install -d $(DESTDIR)/usr/share/perl5/
	install -m 755 $(CURDIR)/timetail $(DESTDIR)/usr/bin/timetail
	install -m 755 $(CURDIR)/timesplit $(DESTDIR)/usr/bin/timesplit
	install -m 755 $(CURDIR)/timecut $(DESTDIR)/usr/bin/timecut
	install -m 755 $(CURDIR)/mymtail.sh $(DESTDIR)/usr/bin/mymtail.sh
	install -m 644 $(CURDIR)/lib/TimeUtils.pm $(DESTDIR)/usr/share/perl5/TimeUtils.pm
	install -m 644 $(CURDIR)/lib/TimeTail.pm $(DESTDIR)/usr/share/perl5/TimeTail.pm

