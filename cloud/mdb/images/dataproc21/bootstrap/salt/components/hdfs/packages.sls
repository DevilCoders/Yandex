hdfs_packages:
    pkg.installed:
        - refresh: False
        - retry:
            attempts: 5
            until: True
            interval: 60
            splay: 10
        - pkgs:
            - hadoop-hdfs
            - hadoop-hdfs-namenode
            - hadoop-hdfs-secondarynamenode
            - hadoop-hdfs-datanode
