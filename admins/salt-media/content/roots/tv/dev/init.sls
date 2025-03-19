mysql -e "stop slave; reset slave all;":
  cron.present:
    - user: root
    - minute: 0
    - hour: 5