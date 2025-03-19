{% if salt.pillar.get('data:dbaas:vtype') == 'compute' %}
{%     if salt.pillar.get('data:dbaas:cluster_id') %}
/etc/ferm/conf.d/10_user_net_routes.conf:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/conf.d/10_user_net_routes.conf
        - template: jinja
        - mode: 644
        - require:
            - test: ferm-ready
        - watch_in:
            - cmd: reload-ferm-rules
{%     else %}
/etc/ferm/conf.d/10_user_net_routes.conf:
    file.absent
{%     endif %}
{% endif %}
