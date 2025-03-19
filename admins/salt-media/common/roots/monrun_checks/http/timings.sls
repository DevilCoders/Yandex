{% for name in salt['pillar.get']('monrun_checks:http:timings', '') %}
{% set check = name.keys()[0] %}
{{ check }}:
{% set time = name[check].get('time', '60') %}
{% set type = name[check].get('type', 'common') %}
{% set pattern = name[check].get('pattern', '/var/log/nginx/access.log') %}
{% set field = name[check].get('field', '(NF-6)') %}
{% set threshold = name[check].get('threshold', '1') %}

  monrun.present:
    - command: /usr/sbin/http-timing.rb {{ time }} {{ type }} '{{ pattern }}' '{{ field }}' {{ threshold }}
{% for mopt in name[check].get('mopts', '') %}
    - {{ mopt }}: {{ name[check]['mopts'][mopt] }} 
{% endfor %}
{% endfor %}

yandex-monitoring-timing-http-check:
  pkg.installed
