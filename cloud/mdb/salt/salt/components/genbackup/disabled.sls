/etc/cron.d/genbackup:
    file.absent

/etc/yandex/genbackup:
    file.absent

/etc/yandex/genbackup-restore:
    file.absent

/etc/logrotate.d/genbackup:
    file.absent

/usr/local/yandex/genbackup_wrapper.sh:
    file.absent

/usr/local/yandex/trust_all_imported_keys.sh:
    file.absent

/usr/local/yandex/import_all_keys.sh:
    file.absent
