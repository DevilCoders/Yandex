{% for name in salt['pillar.get']('monrun_checks:http:ping', '') %}
{% set check = name.keys()[0] %}
{{ check }}:
{% set host = name[check].get('host', check) %}
{% set port = name[check].get('port', '80') %}
{% set timeout = name[check].get('timeout', '3') %}
{% set scheme = name[check].get('scheme', 'http') %}
{% set url = name[check].get('url', '/ping') %}
{% set server = name[check].get('server', 'localhost') %}
{% set expect = name[check].get('expect', 'code: 200') %}
{% set curloptions = name[check].get('curloptions', '-k') %}
{% set antiflap = name[check].get('antiflap', '1') %}

  monrun.present:
    - command: /usr/bin/jhttp.sh -n {{ host }} -p {{ port }} -t {{ timeout }} -s {{ scheme }} -u "{{ url }}" -r {{ server }} -o "{{ curloptions }}" -f {{ antiflap }}
{% for mopt in name[check].get('mopts', '') %}
    - {{ mopt }}: {{ name[check]['mopts'][mopt] }} 
{% endfor %}
{% endfor %}

yandex-juggler-http-check:
  pkg.installed
