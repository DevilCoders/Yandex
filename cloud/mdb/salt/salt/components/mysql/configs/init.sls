{% set osrelease = salt['grains.get']('osrelease') %}
/home/mysql/.ssh:
  file.recurse:
    - user: mysql
    - group: mysql
    - dir_mode: 700
    - file_mode: 600
    - template: jinja
    - source: salt://components/mysql/conf/ssh
    - require:
      - user: mysql-user

{% if salt['pillar.get']('data:mysql:use_ssl', True) %}
/home/monitor/allCAs.pem:
  file.managed:
    - user: monitor
    - group: monitor
    - mode: 400
    - contents_pillar: cert.ca
{% endif %}

include:
    - .mysql_my_cnf
    - .my_cnf
    - .monitor_my_cnf
    - .root_my_cnf

extend:
    /etc/mysql/my.cnf:
        file.managed:
            - require:
                - pkg: mysql
                - user: mysql-user
    /home/monitor/.my.cnf:
        file.managed:
            - require:
                - user: mysql-user
    /home/mysql/.my.cnf:
        file.managed:
            - require:
                - user: mysql-user

/etc/mysql/conf.d:
    file.absent:
      - require:
        - pkg: mysql

/etc/logrotate.d/mysql-common:
    file.managed:
        - source: salt://components/mysql/conf/mysql.logrotate
        - template: jinja
        - mode: 644

/etc/logrotate.d/percona-xtradb-cluster-server-5.7:
    file.absent:
        - require:
            - pkg: mysql

/etc/security/limits.d/80-mysql.conf:
    file.managed:
        - source: salt://components/mysql/conf/limits.conf
        - mode: 644
        - user: root
        - group: root
        - makedirs: True
        - require_in:
            - pkg: mysql
