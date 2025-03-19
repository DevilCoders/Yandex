{% if salt['ydputils.check_roles'](['masternode']) %}
service-spark-history-server:
    service.running:
        - enable: false
        - name: spark@history-server
        - require:
            - pkg: spark_packages
            - file: /etc/spark/conf/spark-defaults.conf
            - file: /etc/spark/conf/spark-env.sh
        - watch:
            - pkg: spark_packages
            - file: /etc/spark/conf/spark-defaults.conf
            - file: /etc/spark/conf/spark-env.sh
 {% if 'hive' in salt['pillar.get']('data:services', []) %}
            - hadoop-property: /etc/hive/conf/hive-site.xml
{% endif %}
{% endif %}
