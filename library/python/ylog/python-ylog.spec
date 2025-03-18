%{!?python_sitelib: %define python_sitelib %(%{__python} -c "from distutils.sysconfig import get_python_lib; print get_python_lib()")}

Name:           python-ylog
Version:        0.1
Release:        1%{?dist}
Summary:        Yandex-specific Python logging utilities
Group:          Development/Libraries
License:        Yandex
URL:            http://www.yandex.ru
Source0:        ylog-%{version}.tar.gz
BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

BuildArch:      noarch

BuildRequires:  python-devel
%if 0%{?fedora} && 0%{?fedora} <= 12
BuildRequires:  python-setuptools-devel
%else
BuildRequires:  python-setuptools
%endif

Requires:       python-setuptools, python >= 2.4

%description
Yandex-specific Python logging utilities


%prep
%setup -q -n ylog-%{version}
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
* Mon May 30 2011 Mikhail Belov <dedm@yandex-team.ru> 0.1
- Initial release for RHEL 
