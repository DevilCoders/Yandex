# if you make changes, the it is advised to increment this number, and provide
# a descriptive suffix to identify who owns or what the change represents
# e.g. release_version 2.MSW
%define release_version 62

# if you wish to compile an rpm without ibverbs support, compile like this...
# rpmbuild -ta glusterfs-1.3.8pre1.tar.gz --without ibverbs
#%define with_ibverbs %{?_without_ibverbs:0}%{?!_without_ibverbs:1}

Summary: Common Monitoring Enviroment
Name: config-monitoring-common
Version: 1.0
Release: %release_version
License: GPL2
Group: System Environment/Base
Vendor: yandex
Packager: tsipa@yandex-team.ru
BuildRoot: %_tmppath/%name-root
BuildArch: noarch
Requires: procps
Requires: yandex-lib-autodetect-environment
URL: https://wiki.yandex-team.ru/DljaAdminov/paysys/PaysysSVN
Source: config-monitoring-common.tar.gz
%description
Common Monitoring Enviroment

%prep 
%setup -n %name

%build 

%install
%{__rm} -rf $RPM_BUILD_ROOT/*
%{__make} install DESTDIR=$RPM_BUILD_ROOT

%files 
%defattr(755,root,root)
/usr/lib/config-monitoring-common/*
%defattr(644,root,root)
%config(noreplace) /etc/monrun/conf.d/*

%changelog
* Wed Oct 15 2014 Antonio Kless <rc5hack@yandex-team.ru> - 1.0-62
- hwerrs.sh - add ERST (error record serialization table) initialization to ignore pat
* Fri Sep 12 2014 Rashit Azizbaev <syndicut@yandex-team.ru> - 1.0-61
- Added yandex-lib-autodetect-environment requirement to spec file
- moved postfix queue check to config-monrun-postfix-check=0.3
- added monrun cfg from mailq
- added monrun cfg from mailq
- Added mail queue monitoring
- config-monitoring-common: remove examples from the repo
- config-monitoring-common: Conflicts: and Provides: config-monitoring-corba-la-check
- Показываю la на kvm хостах
- disable la on virtual hosts
- opened_rotated_logs improved matching
- we don't match something like "catalog" anymore
- changelog
- + checks la and opened logs
- mysql_replica check as added
* Fri Feb 14 2014 Roman Anikin <squirrel@yandex-team.ru> - 1.0-53
- Removed logrotate double check
* Fri Jan 10 2014 Roman Anikin <squirrel@yandex-team.ru> - 1.0-34
- Logrotate double check
* Tue Apr 09 2013 Alexey Simakov <asimakov@yandex-teem.ru> - 1.0
- execution_interval reduced to 60 seconds
- local dns check added (1.0-33)
- fixed Can"t parse output from script (1.0-31)
- fix ncq_enable check (skip devices with size = 0) (1.0-29)
- '[migration/23]' -> '/23]' bug in watchdog module (1.0-28) 
- rm monrun config mtu_diff.conf (1.0-25)
- rm mtu check, rm remove logrotate status file (1.0-24)
- add new logic for logrotate check. (1.0-23)
- 'common' type id specified for all checks (1.0-22)
- added execution_timeout=30 for all checks (1.0-21)
- add new checks: mtu_diff, cron, logrotate and ncq_enable (1.0-19)
* Wed Oct 14 2011 Alexey Simakov <asimakov@yandex-team.ru> - 1.0
- crit_pattern for hw_errs.sh extended  (conntrack:\ table\ full)
- added yandex-lib-autodetect-environment to dependecies
- fixed raid.sh output parsing errs in case of empty /proc/mdadm
- removed stdout of df to avoid "df: no file systems processed" and output parsing errs
- raid,ntp,hw_errs: return true, if yandex-lib-autodetect-environment exist
  and detect openvz container
- raid.sh - devnulling spam like 'mdadm: metadata format XXXXX unknown,
  ignored'
- hwerrs.sh - add EDID (vga cards) to ignore pat

* Wed May 25 2011 Alexey Simakov <asimakov@yandex-team.ru> - 1.0
- mrdaemon.conf deleted

* Wed May 25 2011 Alexey Simakov <asimakov@yandex-team.ru> - 1.0
- monrun configs fixed

* Wed May 25 2011 Alexey Simakov <asimakov@yandex-team.ru> - 1.0
- Initial spec file

* Tue Apr 22 2011 Yudin Sergey <tsipa@yandex-team.ru> - 1.0
- Initial spec file

