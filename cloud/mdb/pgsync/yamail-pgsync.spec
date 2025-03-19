%define _builddir .
%define _sourcedir .
%define _specdir .
%define _rpmdir .

Name: yamail-pgsync
Version: %{_defined_build_number}
Release: %{_defined_release}

Summary: Meta package for pgsync
License: PostgreSQL License
Packager: Borodin Vladimir <d0uble@yandex-team.ru>
Group: System Environment/Meta
Distribution: Red Hat Enterprise Linux

Requires: python-psycopg2 >= 2.6.1
Requires: python-daemon >= 1.6
Requires: python-lockfile >= 0.9
Requires: python-setuptools
Requires: python-kazoo >= 1.3

BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root
BuildArch: noarch

%description
https://wiki.yandex-team.ru/mail/pg/pgsync

%install
%{__rm} -rf %{buildroot}
python setup.py install --root=$RPM_BUILD_ROOT --record=INSTALLED_FILES -O1
cd static && make DESTDIR=$RPM_BUILD_ROOT install
%{__install} -d %{buildroot}/etc/pgsync/plugins

%clean
rm -rf $RPM_BUILD_ROOT

%files -f INSTALLED_FILES
%defattr(0755, root, postgres)
/etc/init.d/pgsync
/etc/logrotate.d/pgsync
%dir /etc/pgsync/plugins
%attr(0400, root, root) /etc/sudoers.d/pgsync
%attr(0644, root, root) /etc/cron.d/wd-pgsync
%attr(0744, root, root) /etc/cron.yandex/wd_pgsync.py*

%changelog
* Wed Apr 22 2015 Vladimir Borodin <d0uble@yandex-team.ru>
- Initial rpm
