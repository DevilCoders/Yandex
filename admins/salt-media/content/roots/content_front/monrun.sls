{% set monlist = ['tv_499', 'tv_5xx', 'tv_timings', 'pda_tv_499', 'pda_tv_5xx'] %}
{% set yaenv = grains['yandex-environment'] %}

/etc/monrun/conf.d/monrun_499_5xx.conf:
  file.absent

yandex-media-py-log-processor:
  pkg.installed

{% for name in monlist %}
/etc/yandex-media-py-log-processor/{{name}}.json:
  file.managed:
    - user: root
    - group: root
    - mode: 0644
    - template: jinja
    - context:
        yaenv: {{yaenv}}
    - source: salt://{{ slspath }}/files/monrun_by_handle/{{name}}.json

monrun_{{name}}:
  monrun.present:
    - name: {{name}}
    - type: tv
    - command: /usr/sbin/py-log-processor -c /etc/yandex-media-py-log-processor/{{name}}.json
    - watch_in:
      - cmd: regenerate-monrun-tasks

# remove old files with new_ prefix
/etc/yandex-media-py-log-processor/new_{{name}}.json:
  file.absent

/etc/monrun/conf.d/new_{{name}}.conf:
  file.absent

{% endfor %}

regenerate-monrun-tasks:
  cmd.wait:
    - name: regenerate-monrun-tasks
