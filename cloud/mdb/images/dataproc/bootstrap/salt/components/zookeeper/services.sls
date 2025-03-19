{% import 'components/hadoop/macro.sls' as m with context %}
{% set roles = m.roles() %}

{% if salt['ydputils.check_roles'](['masternode']) %}
service-zookeeper-server:
    service.running:
        - enable: false
        - name: zookeeper-server
        - require:
            - pkg: zookeeper_packages
            - file: zookeeper-config-myid
        - watch:
            - pkg: zookeeper_packages
            - file: /etc/zookeeper/conf/zoo.cfg
{% endif %}
