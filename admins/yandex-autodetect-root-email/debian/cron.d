#
# fix /etc/aliasess (set valid root email for project-mashine)
#
15 2 * * * root /bin/bash /usr/local/sbin/autodetect_root_email.sh >/dev/null;


