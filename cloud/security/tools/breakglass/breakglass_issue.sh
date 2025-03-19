#!/bin/sh

set -e

if [ "$#" -ne 2 ]; then
    echo "Usage: ${0} CLOUDINC-XXX staff_login"
    exit 1
fi

TICKET="${1}"
REQUESTER="${2}"
KEYDIR="${HOME}/duty/softkey/${TICKET}"
YAV="yav"
YUBICLI="${HOME}/bin/yc-yubikey-cli"

mkdir -p "${KEYDIR}"

# generate key without passphrase
ssh-keygen -b 4096 -t rsa -q -N "" -f session-soft
# generate certificate for 1 day
"${YUBICLI}" piv issue-soft-session-cert --public-key session-soft.pub -r breakglass --ttl 1 --ticket "${TICKET}" --comment "${REQUESTER}"

SSH_META="$(ssh-keygen -L -f session-soft-cert.pub)"
VALID="$(echo "${SSH_META}" | grep Valid | xargs)"

# put in yav with 3 years auto expiration
"${YAV}" create secret "ssh-soft-cert_breakglass_${TICKET}" \
 -c "${VALID}" -t 'ssh_key' --ttl 94694400 \
 -r 'owner:abc:ycsecurity:administration' \
    "reader:user:${REQUESTER}" \
 -f 'session-soft=session-soft' \
    'session-soft.pub=session-soft.pub' \
    'session-soft-cert.pub=session-soft-cert.pub'

echo "${SSH_META}"
echo
echo "Please run the folowwing command if key was successfully uploaded to YAV:\n\t rm ${KEYDIR}/session-soft"
