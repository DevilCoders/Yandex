%define _builddir   .
%define _sourcedir  .
%define _specdir    .
%define _rpmdir     .
%define _unpackaged_files_terminate_build 0

Name:       yamail-monrun-errata
Version:    %{_defined_version}
Release:    %{_defined_release}

Summary:    Yandex mail monrun check for rhel errata updates
License:    Yandex License
Packager:   Dyukov Evgeny <secwall@yandex-team.ru>
Group:      System Environment/Base
Distribution:   Red Hat Enterprise Linux
BuildArch:      noarch

Requires:  monrun >= 1.2.50-2
Requires:  python-argparse

BuildRoot:  %{_tmppath}/%{name}-%{version}-%{release}-root

%description
Yandex mail monrun check for errata security updates

%install
%{__rm} -rf %{buildroot}
%{__install} -d %{buildroot}/usr/local/yandex/monitoring
%{__install} -d %{buildroot}/etc/monrun/conf.d
%{__install} -m644 conf.d/* %{buildroot}/etc/monrun/conf.d
%{__install} -m755 scripts/* %{buildroot}/usr/local/yandex/monitoring

%clean
%{__rm} -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%config /etc/monrun/conf.d/*.conf
/usr/local/yandex/monitoring/*.*

%changelog
* Wed Apr 22 2015 Dyukov Evgeny <secwall@yandex-team.ru>
- initial yandex's rpm build
