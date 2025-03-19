Данная тула предназначена для заливки бекапа YDB (на файловой системе) в YT.

Это не бекап базы в YT! Это экспорт части данных в YT, где источником для экспорта
является предварительно сделанный бекап базы на файловую систему.


Пример запуска тулы:

```
YT_PROXY=hahn ./iam_backup
 --yt-directory //home/cloud/iam/export/preprod
 --fs-directory //home/varkasha/git/iam-backup/ydb_backup
 --table identity/r3/clouds clouds id
 --table identity/r3/folders folders id
 --table scms/access_keys access_keys folder_id,id,key_id
 --table identity/r3/subjects/service_accounts service_accounts id,cloud_id,folder_id
```

Если требуется отгрузить все столбцы таблицы - указать пустую строку. Пример:
```
YT_PROXY=hahn ./iam_backup
 --yt-directory //home/cloud/iam/export/preprod
 --fs-directory //home/varkasha/git/iam-backup/ydb_backup
 --table identity/r3/clouds clouds ""
```
