{% if salt['ydputils.check_roles'](['masternode']) %}
hbase-masternode_packages:
    pkg.installed:
        - refresh: False
        - pkgs:
            - hbase-master
            - hbase-rest
            - hbase-thrift
        - require:
            - pkg: hbase_packages
            - hadoop-property: /etc/hbase/conf/hbase-site.xml
            - file: /etc/hbase/conf/regionservers
            - service: service-zookeeper-server
            - file: patch-/etc/hbase/conf/hbase-env.sh
            - dataproc: hdfs-available

service-hbase-master:
    service:
        - running
        - enable: true
        - name: hbase-master
        - require:
            - pkg: hbase-masternode_packages
            - dataproc: hdfs-available
        - watch:
            - pkg: hbase_packages
            - pkg: hbase-masternode_packages
            - hadoop-property: /etc/hbase/conf/hbase-site.xml
            - file: patch-/etc/hbase/conf/hbase-env.sh

service-hbase-rest:
    service:
        - running
        - enable: true
        - parallel: true
        - name: hbase-rest
        - require:
            - pkg: hbase-masternode_packages
            - hadoop-property: /etc/hbase/conf/hbase-site.xml
            - service: service-hbase-master
        - watch:
            - pkg: hbase_packages
            - pkg: hbase-masternode_packages
            - hadoop-property: /etc/hbase/conf/hbase-site.xml

service-hbase-thrift:
    service:
        - running
        - enable: true
        - parallel: true
        - name: hbase-thrift
        - require:
            - pkg: hbase-masternode_packages
            - hadoop-property: /etc/hbase/conf/hbase-site.xml
            - service: service-hbase-master
        - watch:
            - pkg: hbase_packages
            - pkg: hbase-masternode_packages
            - hadoop-property: /etc/hbase/conf/hbase-site.xml

{% endif %}
{% if salt['ydputils.check_roles'](['datanode', 'computenode']) %}
hbase-computenode_packages:
    pkg.installed:
        - refresh: False
        - pkgs:
            - hbase-regionserver
        - require:
            - pkg: hbase_packages
            - hadoop-property: /etc/hbase/conf/hbase-site.xml
            - file: patch-/etc/hbase/conf/hbase-env.sh

service-hbase-regionserver:
    service:
        - running
        - enable: true
        - name: hbase-regionserver
        - require:
            - pkg: hbase-computenode_packages
            - hadoop-property: /etc/hbase/conf/hbase-site.xml
            - file: patch-/etc/hbase/conf/hbase-env.sh
        - watch:
            - pkg: hbase_packages
            - pkg: hbase-computenode_packages
            - hadoop-property: /etc/hbase/conf/hbase-site.xml
            - file: patch-/etc/hbase/conf/hbase-env.sh
{% endif %}
