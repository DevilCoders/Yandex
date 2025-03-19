%define _builddir	.
%define _sourcedir	.
%define _specdir	.

Name:		config-autodetect-active-eth
Version:	1.0
Release:	41

Summary:	autodetect active ethX iface in system (in non-bridge and bridge mode)
License:	Yandex License
Group:		System Environment/Meta
Distribution:	Red Hat Enterprise Linux
URL:		https://svn.yandex-team.ru/corba-configs/trunk/config-autodetect-active-eth/

BuildRoot:	%{_tmppath}/%{name}-%{version}-%{release}-root
BuildArch:	noarch

Requires:	bridge-utils
Requires:	lshw
Requires:	ethtool
Requires:	yandex-lib-autodetect-environment



%description
autodetect active ethX iface in system (in non-bridge and bridge mode)


%install
#%{__rm} -rf %{buildroot}
make DESTDIR=%{buildroot} install-rpm


%clean
rm -rf $RPM_BUILD_ROOT


%post
exit 0


%files
%defattr(-, root, root)
%{_sysconfdir}/monrun/conf.d/*
%{_usr}/bin/*
%{_usr}/local/sbin/*



%changelog
* Fri May 22 2015 Avramenko Evgeniy <kanst9@yandex-team.ru>
- initial yandex's rpm build
