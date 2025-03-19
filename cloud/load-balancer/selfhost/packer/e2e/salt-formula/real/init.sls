{% set version = salt['grains.get']('e2e_real:version') %}
yc-loadbalancer-e2e-real:
  pkg.installed:
    - name: yc-loadbalancer-e2e-real
    - version: {{ version }}
  service.running:
    - require:
      - pkg: yc-loadbalancer-e2e-real

/etc/yc/loadbalancer-e2e-real/config.yaml:
  file.managed:
    - source: salt://{{ slspath }}/files/config.yaml.j2
    - template: jinja
    - require:
      - pkg: yc-loadbalancer-e2e-real
    - watch_in:
      - service: yc-loadbalancer-e2e-real

/etc/systemd/system/yc-loadbalancer-e2e-real.service.d:
  file.directory:
    - user: root
    - group: root
    - mode: 755
    - require:
      - pkg: yc-loadbalancer-e2e-real

/etc/systemd/system/yc-loadbalancer-e2e-real.service.d/override.conf:
  file.managed:
    - source: salt://{{ slspath }}/files/yc-loadbalancer-e2e-real.override
    - user: root
    - group: root
    - mode: 644
    - require:
      - pkg: yc-loadbalancer-e2e-real
      - file: /etc/systemd/system/yc-loadbalancer-e2e-real.service.d
    - watch_in:
      - service: yc-loadbalancer-e2e-real

nginx:
  pkg.installed:
    - pkgs:
      - libperl-dev
      - nginx
  service.running:
    - enable: True

/etc/nginx/nginx.conf:
  file.managed:
    - source: salt://{{ slspath }}/files/nginx.conf
    - template: jinja
    - require:
      - pkg: nginx
