#!/bin/bash -e
sed -i 's@baseurl=.*@baseurl=https://download.opensuse.org/distribution/leap/42.3/repo/oss@g' /etc/zypp/repos.d/openSUSE-Leap-42.3-0.repo
zypper --gpg-auto-import-keys refresh
zypper --non-interactive install python-Jinja2

zypper --non-interactive install cloud-init

systemctl enable cloud-init
systemctl enable cloud-final
systemctl enable cloud-init-local
systemctl enable cloud-config

echo 'datasource_list: [ NoCloud, GCE, Ec2, None ]' | tee /etc/cloud/cloud.cfg.d/90_datasource_list.cfg

cat > /etc/cloud/cloud.cfg.d/01_cloud.cfg <<EOF
manage_etc_hosts: true
EOF

sed -i 's/GRUB_CMDLINE_LINUX_DEFAULT=.*/GRUB_CMDLINE_LINUX_DEFAULT="net.ifnames=0 showopts console=ttyS0"/g' /etc/default/grub
grub2-mkconfig -o /boot/grub2/grub.cfg
