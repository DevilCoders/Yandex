%define _builddir	.
%define _sourcedir	.
%define _specdir	.

Name:		yandex-media-common-salt-auto
Version:	1.27
Release:	0

Summary:	masterless salt for master of salt
License:	Yandex License
Group:		System Environment/Meta
Distribution:	Red Hat Enterprise Linux
URL: 		svn+ssh://svn.yandex.ru/corba-configs/trunk/yandex-media-common-salt-auto

BuildRoot:	%{_tmppath}/%{name}-%{version}-%{release}-root
BuildArch:	noarch

Requires:	git
Requires:	salt
Requires:	salt-minion
Requires:	cauth-agent
Requires:	salt-yandex-components

%description
masterless salt for master of salt


%install
#%{__rm} -rf %{buildroot}
make DESTDIR=%{buildroot} install -f src-rpm/Makefile


%clean
rm -rf $RPM_BUILD_ROOT


%post
exit 0


%files
%defattr(-, root, root)
%{_sysconfdir}/yandex/salt/minion.tpl
%{_usr}/bin/salt_update


%changelog
* Wed Apr 27 2016 Avramenko Evgeniy <kanst9@yandex-team.ru>
- initial yandex's rpm build
