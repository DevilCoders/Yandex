{% set env = salt['pillar.get']('yandex:environment', 'prod') %}
{% set osmajor = salt['grains.get']('osmajorrelease') | string %}

{% load_yaml as repos %}
'18':
    - apt.bionic
    - apt.mdb-bionic.stable
{% endload %}
{% load_yaml as testing_repos %}
'18':
    - apt.mdb-bionic.testing
{% endload %}
{% load_yaml as dev_repos %}
'18':
    - apt.mdb-bionic.unstable
{% endload %}

include:
    - .internal-ca
    - components.repositories.apt
    - .pkgs_ubuntu
    - .mail
    - .locale
{% if ( (salt['grains.get']('virtual', '') == 'physical'
         and salt['grains.get']('virtual_subtype', None) != 'Docker' )
       or salt['grains.get']('manufacturer', '') == 'OpenStack Foundation') %}
    - .hbf
{% endif %}
{% if salt['grains.get']('virtual', '') == 'physical' and salt['grains.get']('virtual_subtype', None) != 'Docker' %}
    - components.hw-watcher
    - components.wall-e
{% elif salt['grains.get']('virtual_subtype', None) == 'Docker' %}
    - .hostname
{% endif %}
    - components.atop
    - .cores
    - .dns
    - .etc-hosts
    - .time
    - .misc
    - .yandex-environment
    - .dbaas
{% if salt['pillar.get']('data:use_monrun', True) %}
    - components.monrun2
{% endif %}
    - components.repositories
{% for repo in repos[osmajor] %}
    - components.repositories.{{ repo }}
{% endfor %}
{% if salt['pillar.get']('data:dbaas:vtype') != 'compute' and salt['pillar.get']('data:cauth_use', True) %}
    - components.cauth
{% endif %}
{% if env == 'dev' or salt['pillar.get']('data:testing_repos') %}
{%   for repo in testing_repos[osmajor] %}
    - components.repositories.{{ repo }}
{%   endfor %}
{% endif %}
{% if env == 'dev' %}
{%   for repo in dev_repos[osmajor] %}
    - components.repositories.{{ repo }}
{%   endfor %}
{% endif %}
    - .database-slice
{% if salt['grains.get']('virtual', '') == 'lxc' and salt['grains.get']('virtual_subtype', None) != 'Docker' %}
    - .database-slice-adjuster
{% endif %}
    - .systemd

/etc/yandex:
    file.directory:
        - user: root
        - group: root
        - mode: 755
