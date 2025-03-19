{% load_yaml as jolokia %}
conf: 'salt://templates/jolokia/conf.yaml'
{% endload %}
{% set jolokia = salt["pillar.get"]("jolokia", jolokia, merge=True) %}

/usr/lib/yandex-graphite-checks/enabled/jolokia.py:
  file.managed:
    - mode: 0755
    - source: salt://templates/jolokia/jolokia.py
    - require:
      - pip: pyjolokia
      - yafile: /etc/monitoring/jolokia.yaml

/etc/monitoring/jolokia.yaml:
  yafile.managed:
    - source: {{jolokia.conf}}

pyjolokia:
  pip.installed:
    - require:
      - pkg: pip_packages

pip_packages:
  pkg.installed:
    - pkgs:
      - python-pip
      - python-setuptools
