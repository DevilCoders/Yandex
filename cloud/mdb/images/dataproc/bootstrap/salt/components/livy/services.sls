service-livy:
    service.running:
        - enable: false
        - name: livy
        - require:
            - pkg: spark_packages
            - pkg: livy_packages
            - file: /etc/spark/conf/spark-defaults.conf
            - file: /etc/spark/conf/spark-env.sh
            - file: /etc/livy/conf/livy.conf
        - watch:
            - pkg: spark_packages
            - pkg: livy_packages
            - file: /etc/spark/conf/spark-defaults.conf
            - file: /etc/spark/conf/spark-env.sh
            - file: /etc/livy/conf/livy.conf

livy-available:
    dataproc.livy_available:
        - require:
            - service: service-livy