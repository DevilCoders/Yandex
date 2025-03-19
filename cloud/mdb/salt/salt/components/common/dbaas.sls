{% if salt['pillar.get']('data:dbaas:cluster_id') %}
/etc/dbaas.conf:
    file.managed:
        - template: jinja
        - source: salt://{{ slspath }}/conf/etc/dbaas.conf

{%     if salt['pillar.get']('tmp-ssh-key') %}
/root/.ssh/authorized_keys2:
    file.append:
        - text:
            - {{ salt['pillar.get']('tmp-ssh-key') }}
{%     endif %}
{% endif %}
