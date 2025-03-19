hive_packages:
    pkg.installed:
        - refresh: False
        - retry:
            attempts: 5
            until: True
            interval: 60
            splay: 10
        - pkgs:
            - hive
            - hive-hcatalog
            - hive-jdbc
            - hive-metastore
            - hive-server2
            - hive-webhcat
            - hive-webhcat-server

