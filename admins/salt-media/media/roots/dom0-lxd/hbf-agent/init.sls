/etc/yandex-hbf-agent/yandex-hbf-agent.conf:
  file.managed:
    - source: salt://{{ slspath }}/yandex-hbf-agent.conf
    - user: root
    - group: root
    - mode: 644
    - template: jinja
    - context:
      password: {{ salt.pillar.get("lxd:secrets:pass") }}

/etc/yandex-hbf-agent/lxd.crt:
  file.managed:
    - contents: {{ salt.pillar.get("lxd:secrets:crt") | json }}
    - user: root
    - group: root
    - mode: 644

/etc/yandex-hbf-agent/lxd.key:
  file.managed:
    - contents: {{ salt.pillar.get("lxd:secrets:key") | json}}
    - user: root
    - group: root
    - mode: 644

