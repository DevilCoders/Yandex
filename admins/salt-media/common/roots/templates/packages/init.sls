{% from slspath + "/map.jinja" import packages with context %}

{% set list = packages.common %}
{% set list = salt.ldmerge.run(packages.project, list) %}
{% set list = salt.ldmerge.run(packages.cluster, list) %}
{% set list = salt.ldmerge.run(packages.saltbuilder, list) %}

{%- if "repos" in pillar %}
include:
  - templates.repos
{%- endif %}

ignore_file:
  file.managed:
    - name: /etc/yandex-pkgver-ignore.d/salt-packages-formula-filtered-pkgs
    - makedirs: True
    - contents: |
        {%- for p in packages.filtered %}
        {{p}}
        {%- endfor %}

config-monitoring-pkgver:
  pkg.installed:
    - require_in:
      - cmd: pkgver_pl_i_y_g
{%- if "repos" in pillar %}
    - require:
      - sls: templates.repos
{%- endif %}

pkgver_pl_i_y_g:
  cmd.run:
    - name: pkgver.pl -i -y -g
    - env: 
      - DEBIAN_FRONTEND: 'noninteractive'
    - require:
      - file: ignore_file
    - unless: pkgver.pl -a

packages:
  pkg.installed:
    - pkgs:
      {%- for package in list %}
      {%- if package is mapping %}
      {%- for p,v in package.iteritems() %}
      {%- if p not in packages.filtered %}
      - {{ package }}
      {%- endif %}
      {%- endfor %}
      {%- elif package not in packages.filtered %}
      - {{ package }}
      {%- endif %}
      {%- endfor %}
    - require:
      - file: ignore_file
      - cmd: pkgver_pl_i_y_g
{%- if "repos" in pillar %}
      - sls: templates.repos
{%- endif %}

{%- if  packages.autoclean %}
clean_apt_archive:
  file.managed:
    - name: /etc/cron.d/apt-clean
    - mode: 644
    - user: root
    - group: root
    - contents: |
        0 0 * * 6 root apt-get clean
{%- endif %}




