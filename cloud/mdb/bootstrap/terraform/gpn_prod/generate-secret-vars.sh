#!/usr/bin/env bash

../control_plane/generation/generate-secrets-vars.sh ver-01eez7qvyqahqee6txvedqnp2z

mdb_admin_token=$(ya vault get version "ver-01ef41dra38mdffck866tzwr87" -o token)
cat << EOF >> secrets.auto.tfvars
mdb_admin_token     = "$mdb_admin_token"
EOF
