#!/bin/bash

cat > /etc/cron.d/first-boot <<EOF
PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:/usr/games:/usr/local/games

@reboot root /bin/bash /setup.sh > /var/log/yc-setup.log 2>&1

EOF

chmod +x /etc/cron.d/first-boot;
