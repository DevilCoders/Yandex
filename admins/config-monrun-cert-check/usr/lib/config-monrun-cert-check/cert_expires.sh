#!/bin/bash

warndays=30
errdays=7

warn=
err=

cert_path="$1"

# Include default cert pathes
. /usr/lib/config-monrun-cert-check/cert_path.sh

if [ x"$cert_path" = "x" ]; then
    cert_path="$default_cert_path"
fi

# Find certs
cert_files=$(find $cert_path -type f 2>/dev/null)

# Include config if available
if [ -f "/etc/config-monrun-cert-check/cert_expires_config.sh" ]; then
    . /etc/config-monrun-cert-check/cert_expires_config.sh
fi

for f in $cert_files; do
    # Check if file is valid cert
    if ! openssl x509 -in "$f" -noout -enddate 2>&1 | grep -q 'unable to load certificate'; then
        # Get domains from cert
        domain=$(openssl x509 -in "$f" -noout -text | perl -lne 'while (/\bDNS:(\S+)\b/g) { ($h = $1) =~ s/\*/a/; push(@d, $h); } } { print $d[rand(scalar(@d))];')
        if [ -n "$domain" ] && [ "$web_check" = "web_check" ]; then
            # Cert has domains, check using connection to webserver
            enddate=$(openssl s_client -connect localhost:443 -servername "$domain" 2>/dev/null </dev/null | openssl x509 -noout -enddate)
            if ! openssl s_client -connect localhost:443 -servername "$domain" 2>/dev/null </dev/null | openssl x509 -noout -checkend $((warndays * 86400)) >/dev/null; then
                warn="${warn:+$warn, }$(basename "$f") expires ${enddate#*=}"
            fi
            if ! openssl s_client -connect localhost:443 -servername "$domain" 2>/dev/null </dev/null | openssl x509 -noout -checkend $((errdays * 86400)) >/dev/null; then
                err="${err:+$err, }$(basename "$f") expires ${enddate#*=}"
            fi
        else
            # Cert does not have domains, check only cert file itself
            enddate=$(openssl x509 -in "$f" -noout -enddate)
            if ! openssl x509 -in "$f" -noout -checkend $((warndays * 86400)) >/dev/null; then
                warn="${warn:+$warn, }$(basename "$f") expires ${enddate#*=}"
            fi
            if ! openssl x509 -in "$f" -noout -checkend $((errdays * 86400)) >/dev/null; then
                err="${err:+$err, }$(basename "$f") expires ${enddate#*=}"
            fi
        fi
    fi
done

if [ -n "$warn" ]; then
    ret=1
    msg="$warn${msg:+, $msg}"
fi

if [ -n "$err" ]; then
    ret=2
    msg="$err${msg:+, $msg}"
fi

echo "${ret:-0};${msg:-Ok}"
