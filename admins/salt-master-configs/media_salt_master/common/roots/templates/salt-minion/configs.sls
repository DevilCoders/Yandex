{% from slspath + "/map.jinja" import salt_minion with context %}
{% if salt_minion.masters_group is defined %}
{% set masters_group = salt_minion.masters_group | string %}
{% else %}
{% set masters_group = salt_minion.project.decode('utf-8') + '-' + salt_minion.senv + '-salt' %}
{% endif %}
include:
  - .services

/etc/salt/minion.d/minion.conf:
  file.managed: 
    - makedirs: True
    - mode: 644
    - template: jinja
    {% if salt_minion.masterless %}
    - source: {{ salt_minion.masterless_config }}
    - context:
      params: {{ salt_minion.masterless_params }}
    {% else %}
    - source: {{ salt_minion.config }}
    - context: 
      masters: {{ salt.conductor.groups2hosts(masters_group) }}
      params: {{ salt_minion.params }}
    {% endif %}
    - watch_in:
      - service: {{ salt_minion.service }}

/usr/local/bin/salt-key-cleanup:
  file.managed:
    - makedirs: True
    - mode: 755
    - source: salt://{{ slspath }}/files/usr/local/bin/salt-key-cleanup

/usr/local/bin/salt-minion-check.sh:
  file.managed:
    - makedirs: True
    - mode: 755
    - source: salt://{{ slspath }}/files/usr/local/bin/salt-minion-check.sh

{% if salt['pillar.get']('disable_monrun') != True and grains['os_family'] == "Debian" %}
salt-minion:
  monrun.present:
    - command: /usr/local/bin/salt-minion-check.sh
    - execution_interval: 300
{% endif %}

{% if salt_minion.miniond %}
/etc/salt/minion.d:
  file.recurse:
    - source: {{ salt_minion.miniond }}
    - makedirs: True
    - file_mode: 644
    - dir_mode: 755
    - watch_in:
      - service: {{ salt_minion.service }}
{% endif %}

{% if salt_minion.masterless %}
{% for env in salt_minion.masterless_config.file_roots.keys() %}
{% for dir in salt_minion.masterless_config.file_roots[env] %}
{{ dir }}:
  file.directory:
    - makedirs: True
{% endfor %}
{% endfor %}

{% for env in salt_minion.masterless_config.pillar_roots.keys() %}
{% for dir in salt_minion.masterless_config.pillar_roots[env] %}
{{ dir }}:
  file.directory:
      - makedirs: True
{% endfor %}
{% endfor %}
{% endif %}
