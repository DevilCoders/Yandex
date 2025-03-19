{% if salt['grains.get']('virtual', 'physical') != 'lxc' and not salt['pillar.get']('data:lxc_used', False)  and salt['grains.get']('virtual_subtype', None) != 'Docker' %}
/etc/cgconfig.d:
  file.directory

/etc/cgconfig.d/gpdb.conf:
  file.managed:
    - source: salt://{{ slspath }}/conf/cgroups/gpdb.conf
    - create: True
    - user: root
    - group: root
    - mode: 0644
    - template: jinja

{% if grains['os'] == 'Ubuntu' %}
/etc/systemd/system/cgconfig.service:
  file.managed:
    - source: salt://{{ slspath }}/conf/cgroups/cgconfig.service
    - template: jinja
    - onchanges_in:
      - module: systemd-reload
{% endif %}

cgconfigparser -l /etc/cgconfig.d/gpdb.conf:
  cmd.run:
    - onchanges:
      - file: /etc/cgconfig.d/gpdb.conf

cgconfig:
  service.running:
    - enable: True
{% endif %}
