# транкейт табличек для очистки билетов от продовых данных после переналивки по субботам в 4 утра (заполняется кроном КП на тестинге отдельно)
afisha_truncate_sql_file:
  file.managed:
    - name: /usr/share/kinopoisk/afisha_truncate.sql
    - source: salt://{{ slspath }}/files/afisha_truncate.sql
    - makedirs: true
    - user: root
    - mode: 644
afisha_truncate_cron_job:
  file.managed:
    - name: /etc/cron.d/kinopoisk-crutch-truncate-afisha
    - contents: "0 7 * * 6 root mysql kinopoisk < /usr/share/kinopoisk/afisha_truncate.sql 2>&1 >> /var/log/cron_truncate_afisha.log"
    - user: root
    - mode: 644
