#!/bin/bash

set -e

echo "Running docker-compose tests"

cd /go/src/a.yandex-team.ru/cloud/mdb/mdb-health/

# Generate code
make generate

# Build service
make build

# Generate whatever is necessary for integration test (certs, keys, etc)
make genclusterkeys gencerts

# Run service in the background. Not the best solution but...
make run &

# Run actual tests
make itest
