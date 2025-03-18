%{!?python_sitelib: %define python_sitelib %(%{__python} -c "from distutils.sysconfig import get_python_lib; print get_python_lib()")}

Name:           python-blackbox
Version:        0.26
Release:        1%{?dist}
Summary:        Python interface to Yandex Blackbox facility
Group:          Development/Libraries
License:        Yandex
URL:            http://www.yandex.ru
Source0:        blackbox-%{version}.tar.gz
BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

BuildArch:      noarch

BuildRequires:  python-devel
%if 0%{?fedora} && 0%{?fedora} <= 12
BuildRequires:  python-setuptools-devel
%else
BuildRequires:  python-setuptools
%endif

Requires:       python-setuptools, python-yenv

%description
Python interface to Yandex Blackbox facility
Small module to deal with blackbox's methods login(), sessionid() and oauth()


%prep
%setup -q -n blackbox-%{version}
cp -p %{SOURCE0} .


%build
%{__python} -c 'import setuptools; execfile("setup.py")' build


%install
[ "%{buildroot}" != "/" ] && rm -rf %{buildroot}
%{__python} -c 'import setuptools; execfile("setup.py")' install -O1 --skip-build --root %{buildroot}


%clean
[ "%{buildroot}" != "/" ] && rm -rf %{buildroot}


%files
%defattr(-,root,root,-)
%{python_sitelib}/*


%changelog
* Mon May 30 2011 Mikhail Belov <dedm@yandex-team.ru> 0.26
- Initial release for RHEL
