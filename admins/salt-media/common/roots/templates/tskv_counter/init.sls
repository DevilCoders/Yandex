{% from slspath + "/map.jinja" import tskv_counter with context %}

{% for config, dst in tskv_counter.get('configs', {}).iteritems() %}
{{ config }}:
  file.managed:
    - source: {{ dst }}
    - user: root
    - group: root
    - mode: 644
    - makedirs: True
    - template: jinja
    - context:
      global: {{ tskv_counter.global|json }}
      types: {{ tskv_counter.types|json }}
{% endfor %}

yandex-media-common-tskv-counter:
  pkg.installed

{% if tskv_counter.local_gr.enabled %}
{{ tskv_counter.local_gr.package }}:
  pkg.installed

{% for config, opts in tskv_counter.local_gr.get('configs', {}).iteritems() %}
{% if opts.get('enabled', False) %}
{% set source = opts.get('src', tskv_counter.local_gr.src) %}
{% set time = opts.get('timetail_time', tskv_counter.local_gr.timetail_time) %}
{% set log = opts.get('log', tskv_counter.local_gr.log) %}
{% set tskv_config = opts.get('tskv_config', tskv_counter.local_gr.tskv_config) %}
{% set args = ' '.join(opts.get('args', tskv_counter.local_gr.args)) %}
/usr/lib/yandex-graphite-checks/enabled/{{ config }}:
  file.managed:
    - source: {{ source }}
    - user: root
    - group: root
    - mode: 0755
    - makedirs: True
    - template: jinja
    - context:
      time: {{ time }}
      log: {{ log }}
      args: {{ args }}
      tskv_config: {{ tskv_config }}
{% endif %}
{% endfor %}
{% endif %}

{% if tskv_counter.local_monrun.enabled %}
{% for config, opts in tskv_counter.local_monrun.get('configs', {}).iteritems() %}
{% if opts.get('enabled', False) %}
{% set time = opts.get('timetail_time', tskv_counter.local_monrun.timetail_time) %}
{% set log = opts.get('log', tskv_counter.local_monrun.log) %}
{% set tskv_config = opts.get('tskv_config', tskv_counter.local_monrun.tskv_config) %}
{% set args = ' '.join(opts.get('args', tskv_counter.local_monrun.args)) %}
{% set mopts = opts.get('mopts', tskv_counter.local_monrun.mopts) %}
{{ config }}:
  monrun.present:
    - command: timetail -n {{ time }} -t tskv {{ log }} | /usr/bin/tskv_counter -c {{ tskv_config }} -o monrun {{ args }}
{% for item in mopts %}
{% for k, v in item.iteritems() %}
    - {{ k }}: {{ v }}
{% endfor %}
{% endfor %}
{% endif %}
{% endfor %}
{% endif %}
