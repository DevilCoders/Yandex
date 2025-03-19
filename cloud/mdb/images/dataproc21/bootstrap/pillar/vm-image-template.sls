---
data:
    presetup: true
    properties:
        hdfs:
            dfs.namenode.https-address: vm-image-template.db.yandex.net:9872
    topology:
        subclusters:
            masternodes:
                name: mastersubcluster
                role: hadoop_cluster.masternode
                hosts: [vm-image-template.db.yandex.net]
            datanodes:
                name: datasubcluster
                role: hadoop_cluster.datanode
                hosts: [vm-image-template.db.yandex.net]
            computenodes:
                name: compute1
                role: hadoop_cluster.computenode
                hosts: [vm-image-template.db.yandex.net]
