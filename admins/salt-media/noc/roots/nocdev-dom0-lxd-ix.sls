include:
  - units.juggler-checks.common
  - units.juggler-checks.common_hw
  - units.dom0-common

/usr/share/yandex-hbf-agent/rules.d/06-internet.v6:
  file.managed:
    - source: salt://files/nocdev-dom0-lxd-ix/06-internet.v6
    - template: jinja

/usr/share/yandex-hbf-agent/rules.d/06-internet.v4:
  file.managed:
    - source: salt://files/nocdev-dom0-lxd-ix/06-internet.v4
    - template: jinja

# for netegress like %nocdev-dom0-lxd-kernel
repo-search-kernel-unstable:
  pkg.installed:
    - refresh: True
/etc/sysctl.d:
  file.recurse:
    - source: salt://files/nocdev-dom0-lxd-kernel/etc/sysctl.d/
/etc/modules-load.d:
  file.recurse:
    - source: salt://files/nocdev-dom0-lxd-kernel/etc/modules-load.d/
{%if 'netegress' in salt['lxd.container_list'](true)|join('')%}
linux-image-4.19.152-38:
  pkg.installed:
    - refresh: True
    - skip_suggestions: True
linux-headers-4.19.152-38:
  pkg.installed:
    - refresh: True
    - skip_suggestions: True
ipt-ipspoof:
  pkg.installed:
    - refresh: True
    - skip_suggestions: True
kmod-ipspoof:
  pkg.installed:
    - refresh: True
    - skip_suggestions: True
{%endif%}
