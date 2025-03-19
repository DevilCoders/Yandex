{% from slspath + "/map.jinja" import lxd with context %}

lxd_service:
{%- if lxd.base.snap.enabled %}
  pkg.installed:
    - name: snapd
  {%- if lxd.base.snap.proxy %}
  cmd.run:
    - name: snap set system proxy.http={{ lxd.base.snap.proxy }} && snap set system proxy.https={{ lxd.base.snap.proxy }}
    - unless: snap list lxd
    - require:
      - pkg: snapd
  {%- endif %}
  file.managed:
    - order: 1
    - name: /etc/yandex-pkgver-ignore.d/lxd
    - contents: |
        lxd
        lxd-client

Install lxd from snap:
  cmd.run:
    - name: snap install lxd
    - unless: snap list lxd

{% set snap_proxy = "http://127.0.0.1:1111" %}
# snap auto updates to latest lxd release, it breaks running containers.
# we disable auto updates to avoid major crash
Disable Canonical proxy:
  cmd.run:
    - order: last
    - name: snap set system proxy.http={{ snap_proxy}} && snap set system proxy.https={{ snap_proxy}}
    - unless: snap get system proxy.http | fgrep {{ snap_proxy}} && snap get system proxy.https | fgrep {{ snap_proxy }}
    - require:
        - cmd: Initialize lxd daemon

{# use deb packages to install lxd #}
{%- else %}
  pkg.installed:
    - pkgs:
    {%- for pkg in lxd.base.packages %}
      - {{ pkg }}
    {%- endfor %}
{%- endif %}
