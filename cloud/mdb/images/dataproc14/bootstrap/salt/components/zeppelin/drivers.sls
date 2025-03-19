/usr/lib/zeppelin/lib/interpreter/postgresql-jdbc4.jar:
    file.symlink:
        - target: /usr/share/java/postgresql-jdbc4.jar
        - require:
            - pkg: zeppelin_packages

/usr/lib/zeppelin/lib/interpreter/mysql-connector-java.jar:
    file.symlink:
        - target: /usr/share/java/mysql-connector-java.jar
        - require:
            - pkg: zeppelin_packages

/usr/lib/zeppelin/lib/interpreter/clickhouse-jdbc.jar:
    file.symlink:
        - target: /usr/lib/clickhouse-jdbc-connector/clickhouse-jdbc.jar
        - require:
            - pkg: zeppelin_packages


{% if 'hive' in salt['pillar.get']('data:services', []) %}
/usr/lib/zeppelin/lib/interpreter/hive-jdbc-standalone.jar:
    file.symlink:
        - target: /usr/lib/hive/jdbc/hive-jdbc-2.3.6-standalone.jar
        - require:
            - pkg: hive_packages
            - pkg: zeppelin_packages
{% endif %}
