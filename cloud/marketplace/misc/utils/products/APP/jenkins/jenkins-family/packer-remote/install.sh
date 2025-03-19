#!/bin/bash

set -e

source /opt/yc-marketplace/yc-welcome-msg.sh
source /opt/yc-marketplace/yc-clean.sh

echo "Install packages ..."
systemd-run --property="After=apt-daily.service apt-daily-upgrade.service" --wait /bin/true
wget -q -O - https://pkg.jenkins.io/debian/jenkins.io.key | sudo apt-key add -
sh -c 'echo deb http://pkg.jenkins.io/debian-stable binary/ > /etc/apt/sources.list.d/jenkins.list'

DEBIAN_FRONTEND=noninteractive apt-get -y update
DEBIAN_FRONTEND=noninteractive apt-get install -y gnupg curl default-jre nginx git

rm /etc/nginx/sites-enabled/default

cat > /etc/nginx/sites-available/jenkins <<'EOF'
server {
 
    listen 80;
    # change jenkins.domain.tld in proxy_redirect aswell
    # server_name jenkins.domain.tld;
     
    location / {
 
      proxy_set_header        Host $host:$server_port;
      proxy_set_header        X-Real-IP $remote_addr;
      proxy_set_header        X-Forwarded-For $proxy_add_x_forwarded_for;
      proxy_set_header        X-Forwarded-Proto $scheme;
 
      # Fix the "It appears that your reverse proxy set up is broken" error.
      proxy_pass          http://127.0.0.1:8080;
      proxy_read_timeout  90;
 
      # replace jenkins.domain.tld with your domain name here aswell
      # proxy_redirect      http://127.0.0.1:8080 https://jenkins.domain.tld;
  
      # Required for new HTTP-based CLI
      proxy_http_version 1.1;
      proxy_request_buffering off;
    }
}
EOF

ln -s /etc/nginx/sites-available/jenkins /etc/nginx/sites-enabled/
systemctl disable nginx

echo iptables-persistent iptables-persistent/autosave_v4 boolean false | debconf-set-selections
echo iptables-persistent iptables-persistent/autosave_v6 boolean false | debconf-set-selections

DEBIAN_FRONTEND=noninteractive apt-get -q -y install iptables-persistent

generate_print_credentials;
on_install;
print_msg;
