%{!?deb_version: %define deb_version %{expand:%%(head -1 debian/changelog | cut -d '(' -f 2 | cut -d '-' -f 1; #))} }
%{!?deb_release: %define deb_release %{expand:%%(head -1 debian/changelog | cut -d '(' -f 2 | cut -d '-' -f 2 | cut -d ')' -f 1)} }

Summary: timetail
Name: yandex-timetail
Version: %deb_version
Release: %deb_release
License: GPL2
Group: System Environment/Base
Vendor: asimakov@yandex-team.ru
Packager: asimakov@yandex-team.ru
BuildRoot: %_tmppath/%name-root
BuildArch: noarch
Requires: coreutils, perl-TimeDate
URL: http://www.yandex.ru/
Source: %name.tar.gz

%description
TimeTail
Summary: Maintenance tools/Monitoring
Group: System Environment/Base

%prep

%setup -n %name

%build

%install
%{__rm} -rf $RPM_BUILD_ROOT
%{__make} install DESTDIR=$RPM_BUILD_ROOT

%files
/usr/bin/timetail
/usr/bin/timecut
/usr/bin/timesplit
/usr/bin/mymtail.sh
/usr/share/perl5/TimeUtils.pm
/usr/share/perl5/TimeTail.pm

%changelog
* Wed Apr 13 2022 Sergey Filimontsev <askort@yandex-team.ru> - 1.0-51
- update

* Wed Mar 22 2017 Sergey Filimontsev <askort@yandex-team.ru> - 1.0-41
- update to debian release

* Thu Mar 12 2015 Anton Kortunov <toshik@yandex-team.ru> - 1.0-37
- updated TSKV parser: https://st.yandex-team.ru/CADMIN-810

* Thu Jul 19 2012 Igor Shishkin <teran@yandex-team.ru - 1.0-23
- -t mulcagate added

* Wed May 12 2010 Oleg Ukhno <olegu@yandex-team.ru> - 1.0-22
- oracle listener regexp added

* Thu Apr 29 2010 Alexey Simakov <asimakov@yandex-team.ru> - 1.0-21
- statbox regexp added

* Sun Feb 28 2010 Alexey Simakov <asimakov@yandex-team.ru> - 1.0-20
- initial rpm build

