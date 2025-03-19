#!/bin/sh

if [ "$#" -ne 2 ]; then
    echo "Usage: ${0} CLOUDINC-XXX sec-XXXXXX"
    exit 1
fi

TICKET="${1}"
SECRETID="${2}"

KEYDIR="${HOME}/duty/breakglass/${TICKET}"
YAV="yav"

mkdir -p "${KEYDIR}"
umask 377
"${YAV}" get version "${SECRETID}" -o session-soft          > "${KEYDIR}/session-soft"
"${YAV}" get version "${SECRETID}" -o session-soft.pub      > "${KEYDIR}/session-soft.pub"
"${YAV}" get version "${SECRETID}" -o session-soft-cert.pub > "${KEYDIR}/session-soft-cert.pub"

echo "Deleting all keys from your SSH_AUTH_SOCK, sorry"
ssh-add -D
ssh-add "${KEYDIR}/session-soft"
# Identity added: session-soft (session-soft)
# Certificate added: session-soft-cert.pub (type=long_lived_softkey;....

ssh-keygen -L -f "${KEYDIR}/session-soft-cert.pub"

echo Use: alias pssh=\"pssh --no-yubikey -u breakglass --bastion-user breakglass\"
