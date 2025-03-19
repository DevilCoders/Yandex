include:
    - components.dbaas-cron

/etc/dbaas-cron/modules/mdbdns_upper.py:
    file.managed:
        - source: salt://components/dbaas-cron/conf/mdbdns_upper.py
        - mode: '0640'
        - user: root
        - group: monitor
        - require:
            - pkg: dbaas-cron
        - watch_in:
            - service: dbaas-cron

{% if salt['pillar.get']('data:cluster_private_key') %}
cid-key:
    file.managed:
        - name: /etc/dbaas-cron/cid_key.pem
        - mode: '0640'
        - template: jinja
        - contents_pillar: data:cluster_private_key
        - user: root
        - group: monitor
{% endif %}
