livy-server_packages:
    pkg.installed:
        - refresh: False
        - require:
            - pkg: spark_packages
            - pkg: livy_packages
            - file: /etc/spark/conf/spark-defaults.conf
            - file: /etc/spark/conf/spark-env.sh
            - file: /etc/livy/conf/livy.conf
            - file: /etc/default/livy-server
        - pkgs:
            - livy-server

service-livy:
    service:
        - running
        - enable: true
        - name: livy-server
        - require:
            - pkg: spark_packages
            - pkg: livy_packages
            - pkg: livy-server_packages
            - file: /etc/spark/conf/spark-defaults.conf
            - file: /etc/spark/conf/spark-env.sh
            - file: /etc/livy/conf/livy.conf
            - file: /etc/default/livy-server
        - watch:
            - pkg: spark_packages
            - pkg: livy_packages
            - pkg: livy-server_packages
            - pkg: spark-history_packages
            - file: /etc/spark/conf/spark-defaults.conf
            - file: /etc/spark/conf/spark-env.sh
            - file: /etc/livy/conf/livy.conf
