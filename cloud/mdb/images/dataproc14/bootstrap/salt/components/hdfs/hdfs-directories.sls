{% if salt['ydputils.check_roles'](['masternode']) %}

{% for dir in ['/tmp', '/user', '/var', '/tmp/hadoop-yarn', '/tmp/hadoop-yarn/staging', '/tmp/hadoop-yarn/staging/history'] %}
hdfs-directories-{{ dir }}:
    cmd.run:
        - name: hadoop fs -mkdir -p {{ dir }} && hadoop fs -chmod -R 1777 {{ dir }}
        - unless: hadoop fs -test -e {{ dir }} && hadoop fs -getfacl {{ dir }}  | xargs | grep -F "user::rwx group::rwx other::rwx"
        - runas: hdfs
        - require:
            - pkg: hadoop_packages
            - pkg: hdfs_packages
            - service: hadoop-hdfs-namenode
{% endfor %}
{% endif %}
