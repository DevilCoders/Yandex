{% if salt['pillar.get']('data:dbaas:cluster_id') %}
dbaas-conf-file:
    file.managed:
        - name: 'C:\dbaas.conf'
        - template: jinja
        - source: salt://components/common/conf/etc/dbaas.conf
{% endif %}
