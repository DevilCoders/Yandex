zookeeper-pkg:
  pkg.installed:
    - name: zookeeper

zookeeper.conf:
  file.managed:
    - name: /etc/init/zookeeper.conf
    - source: salt://{{ slspath }}/zookeeper.conf
    - user: root
    - group: root
    - mode: 644

zookeeper:
  service.running:
    - enable: True
    - watch:
      - file: zookeeper.conf
      - file: /etc/zookeeper/conf/zoo.cfg
    - require:
      - file: zookeeper.conf
      - pkg: zookeeper-pkg
  monrun.present:
    - command: nc -vz -w 1 localhost 2181 &>/dev/null; if [ $? == 0 ]; then echo "0;Zk is up"; else echo "2;Zk port is closed"; fi
    - execution_interval: 60

/etc/zookeeper/conf/zoo.cfg:
  file.managed:
    - source: salt://{{ slspath }}/zoo.cfg
    - user: root
    - group: root
    - mode: 644
