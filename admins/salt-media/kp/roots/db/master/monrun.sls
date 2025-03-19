monrun_mysql_too_many_connections:
  monrun.present:
    - name: mysql-too-many-connections
    - command: timetail -t tskv -n 1800 /var/log/mysql/mysql-error.log 2>/dev/null | perl -nE 'if (/Too many connections/) { print "2;Too many connections\n"; exit; } } { print "0;OK\n";'

monrun_pt-stalk:
  monrun.present:
    - name: pt-stalk
    - command: timetail -t tskv -n 3600 /var/log/pt-kill.log 2>/dev/null | perl -ne 'if (/KILL\s\d+/) { print "2;pt-kill killed queries\n"; exit; } } { print "0;OK\n";'
