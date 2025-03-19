serve-expired-from-cache:
  file.managed:
    - name: /etc/unbound/unbound.conf.d/51-serve-expired.conf
    - source: salt://units/unbound-use-cache/files/51-serve-expired.conf
    - mode: 644
    - user: root
    - group: root

  service.running:
    - name: unbound
    - watch:
        - file: /etc/unbound/unbound.conf.d/51-serve-expired.conf
{%- if grains['oscodename'] in ['lucid', 'trusty', 'xenial'] %}
  pkg.installed:
    - name: yandex-unbound
    - version: 1.10.2
{%- endif %}
