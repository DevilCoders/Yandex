spark_packages:
    pkg.installed:
        - refresh: False
        - retry:
            attempts: 5
            until: True
            interval: 60
            splay: 10
        - pkgs:
            - spark-core
            - spark-external
            - spark-python
            # - spark-sparkr
            - spark-datanucleus
            - spark-yarn-shuffle
            - spark-history-server
