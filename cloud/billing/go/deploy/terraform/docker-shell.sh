#!/bin/bash

# remove providers state before clean shell run
find . -type d -name .terraform | xargs -r rm -r
find . -type f -name .terraform.lock.hcl | xargs -r rm

SKM_TOKEN=`yc iam create-token` docker-compose -f tools/docker-compose.yaml run --rm  terragrunt
