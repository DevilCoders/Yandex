{% set any = 'any' %}

{% import 'components/pip/packages.sls' as packages with context %}
{% set packs = packages.config_site['pip'] %}
pip-environment:
  pip.installed:
    - bin_env: /opt/conda/bin/pip3
    - parallel: True
    - pkgs:
      {% for pkg, version in packs.items() %}
        - {{ salt['ydputils.pip_package_version'](pkg, version) }}
      {% endfor %}
    - require:
      - conda: conda-environment
