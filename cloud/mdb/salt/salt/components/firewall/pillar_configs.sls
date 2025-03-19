{% set pillar_file_list = [
    '01_pillar_rules_ip4.conf',
    '01_pillar_rules_ip6.conf',
]%}
# Pillar-defined firewall (INPUT-only)
{% if salt['pillar.get']('data:firewall', False) or salt['pillar.get']('firewall', False) %}
{%   for file in pillar_file_list %}
/etc/ferm/conf.d/{{ file }}:
    file.managed:
        - makedirs: True
        - template: jinja
        - source: salt://{{ slspath }}/conf/conf.d/{{file}}
        - require:
            - test: ferm-ready
        - watch_in:
            - cmd: reload-ferm-rules
{%   endfor %}
{% endif %}
