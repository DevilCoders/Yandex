#!/bin/bash

set -xe

terraform init -backend-config="secret_key=$(ya vault get version sec-01dyfav4qwk78k01k80nby1qaf -o secret_key)"
