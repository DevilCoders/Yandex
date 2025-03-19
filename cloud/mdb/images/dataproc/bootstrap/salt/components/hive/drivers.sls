/usr/lib/hive/lib/postgresql-jdbc4.jar:
    file.symlink:
        - target: /usr/share/java/postgresql-jdbc4.jar 
        - require:
            - pkg: hive_packages

/usr/lib/hive/lib/mysql-connector-java.jar:
    file.symlink:
        - target: /usr/share/java/mysql-connector-java.jar
        - require:
            - pkg: hive_packages

/usr/lib/hive/lib/clickhouse-jdbc.jar:
    file.symlink:
        - target: /usr/lib/clickhouse-jdbc-connector/clickhouse-jdbc.jar
        - require:
            - pkg: hive_packages

/usr/lib/hive/lib/hive-spark-client.jar:
    file.symlink:
        - target: /usr/lib/hive/lib/hive-spark-client-3.1.2.jar
        - require:
            - pkg: hive_packages