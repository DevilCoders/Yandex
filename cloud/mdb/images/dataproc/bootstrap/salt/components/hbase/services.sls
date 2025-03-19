{% if salt['ydputils.check_roles'](['masternode']) %}
service-hbase-master:
    service.running:
        - enable: false
        - name: hbase@master
        - require:
            - pkg: hbase_packages
            - file: /etc/hbase/conf/regionservers
            - hadoop-property: /etc/hbase/conf/hbase-site.xml
            - hadoop-property: /etc/hbase/conf/hbase-policy.xml
            - service: service-zookeeper-server
            - file: patch-/etc/hbase/conf/hbase-env.sh
            - dataproc: hdfs-available
            - cmd: hdfs-directories-/user
        - watch:
            - pkg: hbase_packages
            - hadoop-property: /etc/hbase/conf/hbase-site.xml
            - hadoop-property: /etc/hbase/conf/hbase-policy.xml
            - file: patch-/etc/hbase/conf/hbase-env.sh

service-hbase-rest:
    service.running:
        - enable: false
        - name: hbase@rest
        - require:
            - pkg: hbase_packages
            - file: /etc/hbase/conf/regionservers
            - hadoop-property: /etc/hbase/conf/hbase-site.xml
            - hadoop-property: /etc/hbase/conf/hbase-policy.xml
            - service: service-zookeeper-server
            - file: patch-/etc/hbase/conf/hbase-env.sh
            - dataproc: hdfs-available
        - watch:
            - pkg: hbase_packages
            - hadoop-property: /etc/hbase/conf/hbase-site.xml
            - hadoop-property: /etc/hbase/conf/hbase-policy.xml
            - file: patch-/etc/hbase/conf/hbase-env.sh

hbase-rest-available:
    dataproc.hbase_rest_available:
        - require:
            - service: service-hbase-rest

service-hbase-thrift:
    service.running:
        - enable: false
        - name: hbase@thrift
        - require:
            - pkg: hbase_packages
            - file: /etc/hbase/conf/regionservers
            - hadoop-property: /etc/hbase/conf/hbase-site.xml
            - hadoop-property: /etc/hbase/conf/hbase-policy.xml
            - service: service-zookeeper-server
            - file: patch-/etc/hbase/conf/hbase-env.sh
            - dataproc: hdfs-available
        - watch:
            - pkg: hbase_packages
            - hadoop-property: /etc/hbase/conf/hbase-site.xml
            - hadoop-property: /etc/hbase/conf/hbase-policy.xml
            - file: patch-/etc/hbase/conf/hbase-env.sh

hbase-rs-available:
    dataproc.hbase_rs_available:
        - require:
            - service: service-hbase-master

{% else %}
hbase-master-available:
    dataproc.hbase_master_available
    

service-hbase-regionserver:
    service.running:
        - enable: false
        - name: hbase@regionserver
        - require:
            - pkg: hbase_packages
            - hadoop-property: /etc/hbase/conf/hbase-site.xml
            - file: patch-/etc/hbase/conf/hbase-env.sh
            - dataproc: hbase-master-available
        - watch:
            - pkg: hbase_packages
            - hadoop-property: /etc/hbase/conf/hbase-site.xml
            - file: patch-/etc/hbase/conf/hbase-env.sh
{% endif %}
