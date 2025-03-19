grad-server:
  user.present:
    - name: grad
    - system: True
  pkg.latest:
    - refresh: true
  service.running:
    - enable: True

redis-server:
  pkg.installed: []
  service.running:
    - enable: True

snmp:
  pkg:
    - installed

/etc/grad/grad_server.yml:
  file.managed:
  {%- if grains['yandex-environment'] == 'testing' %}
    - source: salt://{{ slspath }}/files/etc/grad/grad_server_test.yml
  {% else %}
    - source: salt://{{ slspath }}/files/etc/grad/grad_server.yml
  {% endif %}
    - template: jinja
    - user: grad
    - group: grad
    - mode: 600
    - makedirs: True

/etc/telegraf/telegraf.d/telegraf_grad.conf:
  file.managed:
  - source: salt://{{ slspath }}/files/etc/telegraf/telegraf.d/telegraf_grad.conf
  - template: jinja
  - user: telegraf
  - group: telegraf
  - mode: 660
  - makedirs: True

{% if grains['yandex-environment'] != 'testing' %}
{% include slspath + "/production.sls" %}
{% endif %}

/etc/rsyslog.d/60-grad.conf:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/rsyslog.d/60-grad.conf
    - template: jinja

/etc/yandex/unified_agent/conf.d:
  file.recurse:
    - source: salt://{{ slspath }}/files/etc/yandex/unified_agent/conf.d
    - template: jinja
    - user: unified_agent
    - group: unified_agent
    - file_mode: 644
    - dir_mode: 755
    - makedirs: True
    - clean: True

/etc/yandex/unified_agent/secrets/solomon-oauth-token:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/yandex/unified_agent/secrets/solomon-oauth-token
    - template: jinja
    - user: unified_agent
    - group: unified_agent
    - mode: 600
    - makedirs: True
/etc/yandex/unified_agent/secrets/tvm:
  file.managed:
    - user: unified_agent
    - group: unified_agent
    - mode: 400
    - dir_mode: 750
    - makedirs: True
    {%- if grains['yandex-environment'] == 'testing' %}
    - contents: "!!!secret!!!"
    {%- else %}
    - contents_pillar: 'unified_agent:tvm-client-secret'
    {% endif %}
