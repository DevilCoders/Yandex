{% load_yaml as m %}
robot: "robot-media-salt"

{% if "test" in  grains["yandex-environment"] %}
masters_group: "strm-test-salt"
tmpfs: "1G"
{% else %} # production
masters_group: "strm-salt"
tmpfs: "8G"
{% endif %}

{% endload %}

{% load_yaml as old_files %}
- /etc/cron.d/salt-csync2
- /etc/cron.d/salt_sync
- /etc/cron.d/salt-dynamic-roots
- /etc/cron.d/salt-cron-jobs
- /usr/bin/salt_sync
- /etc/sudoers.d/salt-key-cleanup
- /usr/bin/csync2_force_push.sh
- /etc/cron.d/salt-backup
- /usr/bin/salt-backup.sh
- /etc/cron.d/salt-master-manage-down-removekeys
- /etc/nginx/sites-enabled/salt-master.conf
- /etc/salt/pki/master/master_sign.pub
- /etc/salt/pki/master/master_sign.pem
{% endload %}
