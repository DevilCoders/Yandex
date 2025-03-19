/opt/databases/cassandra:
  mount.mounted:
    - device: tmpfs
    - fstype: tmpfs
    - hidden_opts: size=1G,uid=cassandra,gid=cassandra
    - opts: size=1G,uid=cassandra,gid=cassandra
    - dump: 0
    - pass_num: 0
    - persist: True
    - mkmnt: True
    - user: cassandra

/opt/databases/mongodb:
  mount.mounted:
    - device: tmpfs
    - fstype: tmpfs
    - hidden_opts: size=1G,uid=mongodb,gid=mongodb
    - opts: size=1G,uid=mongodb,gid=mongodb
    - dump: 0
    - pass_num: 0
    - persist: True
    - mkmnt: True
    - user: mongodb

change_mysql_dir_owners:
  file.directory:
    - name: /opt/databases/mysql
    - user: mysql
    - group: mysql
    - mode: 755 
    - recurse:
      - user
      - group

{% for dir in 'logs', 'data' %}
/opt/databases/mysql/{{ dir }}:
  mount.mounted:
    - device: tmpfs
    - fstype: tmpfs
    - hidden_opts: size=1G,uid=mysql,gid=mysql
    - opts: size=1G,uid=mysql,gid=mysql
    - dump: 0
    - pass_num: 0
    - persist: True
    - mkmnt: True
    - user: mysql
{% endfor %}


# Directory to build verstka for music
/opt/builder/music:
  file.directory:
    - user: root
    - group: root
    - mode: 750
    - makedirs: True
  mount.mounted:
    - device: tmpfs
    - fstype: tmpfs
    - hidden_opts: size=4G,uid=root,gid=root
    - opts: size=4G,uid=root,gid=root
    - dump: 0
    - pass_num: 0
    - persist: True
    - mkmnt: True
    - require:
      - file: /opt/builder/music

