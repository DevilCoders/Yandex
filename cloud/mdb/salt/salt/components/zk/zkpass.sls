/root/.zkpass:
    file.managed:
        - source: salt://{{ slspath }}/conf/zkpass
        - template: jinja
        - mode: 600
        - user: root
        - group: root

/home/zookeeper/.zkpass:
    file.managed:
        - source: salt://{{ slspath }}/conf/zkpass
        - template: jinja
        - mode: 640
        - user: root
        - makedirs: True
        - group: zookeeper
