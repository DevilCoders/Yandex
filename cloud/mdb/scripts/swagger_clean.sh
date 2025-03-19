#!/bin/bash

set -e

echo "Cleaning swagger-generated code for '${MDB_PROJECT_NAME}'"

if [[ -d 'generated/swagger' ]]; then
    find generated/swagger ! -name 'swagger' ! -name 'generated' ! -name 'restapi' ! -name "configure_*.go" -delete;
fi
