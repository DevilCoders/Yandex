#!/bin/bash
set -e
if [ "$(uname)" == Darwin ]; then
    shopt -s expand_aliases
    alias sed=gsed
fi

pass=7777777
name=$(echo $1 | sed 's/.pfx//')
cert=$(openssl pkcs12 -in ${name}.pfx -nokeys -password pass:${pass} | sed 's/^/    /')
pem=$(openssl pkcs12 -in ${name}.pfx -nocerts -nodes -password pass:${pass}  | sed 's/^/    /')

cat > certs.sls << EOF
certs:
  pem: |
${pem}
  crt: |
${cert}
EOF
