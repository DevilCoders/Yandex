%define _builddir	.
%define _sourcedir	.
%define _specdir	.
%define _rpmdir		.

Name:		python-netconfig
Version:	%(cat debian/changelog | head -1 | cut -f2 -d\( | cut -f1 -d\))
Release:	1

Summary:	The networking configurator based on conductor
License:	Yandex License
Group:		System Environment/Utilities
Distribution:	Red Hat Enterprise Linux

Source0:	src
Source1:        redhat-netconfig

Requires:	python-ipaddr
Requires:       python26-pydns

BuildArch:	noarch

BuildRoot:	%{_tmppath}/%{name}-%{version}-%{release}-root


%description
The networking configurator based on conductor


%prep


%build


%install
%{__rm} -rf %{buildroot}
mkdir -p %{buildroot}/usr/lib/yandex/python-netconfig
mkdir -p %{buildroot}/usr/sbin

cp -a %{SOURCE0}/* %{buildroot}/usr/lib/yandex/python-netconfig
cp -a %{SOURCE1}   %{buildroot}/usr/sbin/netconfig


%clean
rm -rf %{buildroot}

%files
/usr/lib/yandex/python-netconfig
/usr/sbin/netconfig

%post

%preun

%changelog
* Wed Jan 28 2015 Roman Andriadi <nARN@yandex-team.ru>
- Moved from cjson to standard json due to conflicts in Ubuntu repos

* Thu Nov 13 2014 Leonid Borchuk <xifos@yandex-team.ru>
- added internal ip generation for hadoop

* Tue Nov 26 2013 Pavel Pushkarev <paulus@yandex-team.ru>
- initial yandex's rpm build
