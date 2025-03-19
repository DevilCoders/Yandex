YC_WELCOME_DELEMITER="#################################################################"
YC_WELCOME_TITLE="This instance runs Yandex.Cloud Marketplace product"
YC_WELCOME_TTY_TITLE="YC-Marketplace product credentials:"

YC_WELCOME_BODY="Please wait while we configure your product..."
YC_WELCOME_HELP="Documentation for Yandex Cloud Marketplace images available at https://cloud.yandex.ru/docs"

flush_welcome_body() {
YC_WELCOME_BODY=""
}

print_msg() {
cat > /etc/ssh/yc-marketplace-msg <<EOF

${YC_WELCOME_DELEMITER}
${YC_WELCOME_TITLE}
${YC_WELCOME_BODY}

${YC_WELCOME_HELP}

${YC_WELCOME_DELEMITER}

EOF

if [[ ! -f /etc/update-motd.d/99-yc-marketplace ]]
then
    cat > /etc/update-motd.d/99-yc-marketplace <<EOF
#!/bin/bash

echo "";
cat /etc/ssh/yc-marketplace-msg;

EOF

    chmod +x /etc/update-motd.d/99-yc-marketplace
fi

}

print_tty() {
if [ -z $1 ];
then
cat > /dev/ttyS0 <<EOF
${YC_WELCOME_DELEMITER}
${YC_WELCOME_TTY_TITLE}

${YC_WELCOME_BODY}

${YC_WELCOME_DELEMITER}
EOF
else
echo $1 > /dev/ttyS0
fi
}

generate_print_credentials() {
cat > /etc/cron.d/print-credentials <<EOF
PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:/usr/games:/usr/local/games

@reboot root /opt/yc-marketplace/print-credentials.sh

EOF

mkdir -p /opt/yc-marketplace/
cat > /opt/yc-marketplace/print-credentials.sh <<EOF

source /opt/yc-marketplace/yc-welcome-msg.sh

YC_WELCOME_BODY=\$(cat /root/default_passwords.txt)

print_tty
EOF
chmod +x /opt/yc-marketplace/print-credentials.sh
# /root/default_passwords.txt > /dev/ttyS0
}

run_print_credentials() {
    /opt/yc-marketplace/print-credentials.sh || echo 'Failed to print to TTY.'
}
