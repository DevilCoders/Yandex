{% import 'components/hadoop/macro.sls' as m with context %}
{% set roles = m.roles() %}

{% if salt['ydputils.check_roles'](['masternode']) %}
zookeeper-initialize:
    cmd.run:
        - name: . /etc/default/hadoop && . /etc/default/zookeeper && /usr/lib/zookeeper/bin/zkServer-initialize.sh --myid=1
        - unless: test -d /var/lib/zookeeper/version-2
        - runas: zookeeper
        - require:
            - pkg: zookeeper_packages
            - file: zookeeper-config-myid

service-zookeeper-server:
    service.running:
        - enable: false
        - name: zookeeper-server
        - require:
            - pkg: zookeeper_packages
            - file: zookeeper-config-myid
            - cmd: zookeeper-initialize
        - watch:
            - pkg: zookeeper_packages
            - file: /etc/zookeeper/conf/zoo.cfg
{% endif %}
