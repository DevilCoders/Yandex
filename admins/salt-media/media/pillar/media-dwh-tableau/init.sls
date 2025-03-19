tableau_version: 2021.1.2
pg_driver_version: 42.2.14
admin_user: robot-media-tableau
admin_user_password: {{ salt.yav.get('sec-01f6fdbn9tcm6s19hwsm52mgrp[password]') | json }}
cmake_version: 3.14
cmake_patch_version: 7
clickhouse_server: c-0f4cbcdb-cfbf-486d-a89c-2b22439bf811.rw.db.yandex.net
clickhouse_server_afisha_prod: c-mdbm2b2vfbk3ipbbll6n.rw.db.yandex.net
clickhouse_afisha_uploader_password: {{ salt.yav.get('sec-01fw1s6mzy8w0khrr3v46s581k[user.uploader]') | json }}
clickhouse_ott_password: {{ salt.yav.get('sec-01f8dghwtqvaqfkqdmsc777bvx[ottuser]') | json }}
clickhouse_tickets_password: {{ salt.yav.get('sec-01f8dghwtqvaqfkqdmsc777bvx[tickets]') | json }}
afisha_music_yt_token: {{ salt.yav.get('sec-01f0dnqya7zdbv57pdh72f1b2v[yt.token]') | json }}
ott_yt_token: {{ salt.yav.get('sec-01dkxnxwk9heyz5cmwgamd5s6z[nirvana-secret]') | json }}
plus_yt_token: {{ salt.yav.get('sec-01f7azxesnd5emwkw4v56p961h[robot-plus-analytics-yt-token]') | json }}
msmarkt_yt_token: {{ salt.yav.get('sec-01etmndpr69mbj2htb7xfyrd3s[yt]') | json }}
chyt_server: hahn.yt.yandex.net
tableau_installation_lockfile: /opt/tableau_do_not_remove.txt
registration_last_name: Arseniev
registration_title: Y.M. Tableau
registration_department: Mediaservices
registration_first_name: Aleksei
registration_email: afisha-sre@yandex-team.ru
s3_access_key: {{ salt.yav.get('sec-01dkxnxwk9heyz5cmwgamd5s6z[s3.AccessKeyId]') | json }}
s3_secret_key: {{ salt.yav.get('sec-01dkxnxwk9heyz5cmwgamd5s6z[s3.AccessSecretKey]') | json }}
s3_bucket: media-tableau-backups
