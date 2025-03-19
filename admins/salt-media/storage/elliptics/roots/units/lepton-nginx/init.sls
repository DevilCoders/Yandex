include:
  - templates.certificates
  - templates.unistat-lua
  - templates.yasmagent
  - templates.lepton

nginx-site:
  file.managed:
    - name: /etc/nginx/sites-available/10-lepton.conf
    - source: salt://files/elliptics-lepton/10-lepton.conf
    - makedirs: True
    - user: root
    - group: root
    - mode: 644

nginx-lua-conf:
  file.managed:
    - name: /etc/nginx/conf.d/lua.conf
    - source: salt://files/elliptics-lepton/lua.conf
    - makedirs: True
    - user: root
    - group: root
    - mode: 644

nginx-conf:
  file.managed:
    - name: /etc/nginx/include/lepton.conf
    - source: salt://files/elliptics-lepton/lepton.conf
    - makedirs: True
    - user: root
    - group: root
    - mode: 644

nginx-logrotate-conf:
  file.managed:
    - name: /etc/logrotate.d/nginx-access.conf
    - source: salt://files/elliptics-lepton/nginx-access.conf
    - makedirs: True
    - user: root
    - group: root
    - mode: 644

nginx-metrics:
  file.managed:
    - name: /etc/nginx/include/metrics_lepton.lua
    - source: salt://files/elliptics-lepton/metrics_lepton.lua
    - mode: 644
    - user: root
    - group: root

nginx-conf-enabled:
  file.symlink:
    - name: /etc/nginx/sites-enabled/10-lepton.conf
    - target: /etc/nginx/sites-available/10-lepton.conf


