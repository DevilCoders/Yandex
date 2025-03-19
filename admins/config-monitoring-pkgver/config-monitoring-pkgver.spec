%define _builddir	.
%define _sourcedir	.
%define _specdir	.

Name:		config-monitoring-pkgver
Version:	0.4.56
Release:	0

Summary:	Compares installed packages versions with conductor and monitors their compliance
License:	Yandex License
Group:		System Environment/Meta
URL:        svn+ssh://svn.yandex.ru/admin-tools/trunk/config-monitoring-pkgver
Distribution:	Red Hat Enterprise Linux

Requires:	perl-Sys-Hostname-Long
Requires:	perl-Sort-Versions
Requires:	perl-Net-INET6Glue

BuildRoot:	%{_tmppath}/%{name}-%{version}-%{release}-root
BuildArch:	noarch


%description
Compares installed packages versions with conductor and monitors their compliance

%install
#%{__rm} -rf %{buildroot}
make DESTDIR=%{buildroot} install-rpm


%clean
rm -rf $RPM_BUILD_ROOT


%post
exit 0


%files
%defattr(-, root, root)
%{_sysconfdir}/init.d/pkgver
%{_sysconfdir}/logrotate.d/pkgver
%{_sysconfdir}/monrun/conf.d/*.conf
%{_usr}/bin/runlevel_check.sh
%{_usr}/bin/pkgver.pl
%{_usr}/bin/monrun_pkgver.sh



%changelog
* Wed Apr 8 2015 Avramenko Evgeniy <kanst9@yandex-team.ru>
- initial yandex's rpm build
