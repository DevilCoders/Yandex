add_redb_to_hosts:
  cmd.run:
    - name: echo "2a02:6b8:0:3400:0:285:0:6 redb.kp.yandex.net" >> /etc/hosts
    - unless: [[ grep 'redb.kp.yandex.net' /etc/hosts ]]

add_kp-dbs_to_hosts:
  cmd.run:
    - name: echo "2a02:6b8:0:3400:0:285:0:6 kp-dbs.kp.yandex.net" >> /etc/hosts
    - unless: [[ grep 'kp-dbs.kp.yandex.net' /etc/hosts ]]
