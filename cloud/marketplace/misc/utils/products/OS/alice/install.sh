#!/bin/bash
set -e

source /opt/yc-marketplace/yc-welcome-msg.sh
source /opt/yc-marketplace/yc-clean.sh

# initinal setup
apt-get update -qq
apt-get --yes install python-pip python-dev nodejs npm nginx-full certbot

pip install virtualenv

# cloning last version to app dir
mkdir /srv/alice-buy-elephant
git clone https://github.com/yandex/alice-skills.git
cp alice-skills/python/buy-elephant/now/* /srv/alice-buy-elephant
cd /srv/alice-buy-elephant

# setting a wsgi app
cat > wsgi.py <<EOF
from api import app

if __name__ == "__main__":
    app.run()

EOF

virtualenv alice
. alice/bin/activate
pip install -r requirements.txt
pip install uwsgi
deactivate

cat > alice.ini <<EOF
[uwsgi]
module = wsgi:app

master = true
processes = 1

socket = alice.sock
chmod-socket = 660
vacuum = true

die-on-term = true

EOF

chown -R www-data:www-data /srv/alice-buy-elephant

# service for this skill
cat > /etc/systemd/system/alice-test.service <<EOF
[Unit]
Description=uWSGI instance to serve Alice testing skill
After=network.target

[Service]
User=www-data
Group=www-data
WorkingDirectory=/srv/alice-buy-elephant
Environment="PATH=/srv/alice-buy-elephant/alice/bin"
ExecStart=/srv/alice-buy-elephant/alice/bin/uwsgi --ini alice.ini

[Install]
WantedBy=multi-user.target

EOF

# nginx site definition
cat > /etc/nginx/sites-available/alice.conf <<EOF
server {
    listen 80;

    include acme;
    location / {
        include uwsgi_params;
        uwsgi_pass unix:/srv/alice-buy-elephant/alice.sock;
    }
}

EOF

# setting alice skill as home page
ln -s /etc/nginx/sites-available/alice.conf /etc/nginx/sites-enabled/
unlink /etc/nginx/sites-enabled/default

# certbot setup
mkdir -p /var/www/html/.well-known/acme-challenge
chown -R www-data:www-data /var/www/html/.well-known/acme-challenge
cat >> /etc/letsencrypt/cli.ini <<EOF
authenticator = webroot
webroot-path = /var/www/html
post-hook = service nginx reload
text = True

EOF

cat > /etc/nginx/acme <<EOF
location /.well-known {
    root /var/www/html;
}

EOF

# pretty motd
cat > /etc/profile.d/alice.sh <<EOF
#!/bin/bash

alias ls='ls -oh'
alias awk='gawk'

export HISTCONTROL=ignoredups:erasedups  # no duplicate entries
export HISTSIZE=100000                   # big big history
export HISTFILESIZE=100000               # big big history
shopt -s histappend                      # append to history, don't overwrite it
# Save and reload the history after each command finishes
export PROMPT_COMMAND="history -a; history -c; history -r; \$PROMPT_COMMAND"

export PS1='\[\e[1;32m\]\u@\h:\w$\[\e[m\] '

echo "
#####################################################################
Welcome to Yandex.Dialogs Virtual Machine
Image Build: `LC_TIME=en_US.UTF-8 date -u`

Yandex.Dialogs example script locaton:
    /root/alice-skills/python/buy-elephant/now/

Installed packages:
    python-pip, nginx, certbot, uwsgi, nodejs, npm

Use this command to start test service:
    sudo systemctl start nginx
    sudo systemctl start alice-test

    Then you can try it on http://<ip|fqdn>/

Description and help:
    https://tech.yandex.ru/dialogs/alice/doc/about-docpage/
#####################################################################
"

EOF

generate_print_credentials;
on_install;
print_msg;
