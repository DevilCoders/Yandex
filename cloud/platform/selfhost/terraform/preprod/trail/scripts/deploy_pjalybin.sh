#!/bin/sh

# to deploy custom version:
# scripts/deploy_tests.sh -var="application_version=12345-abcdef1234"

# to deploy automatically without approve:
# scripts/deploy_tests.sh -auto-approve


for i in trail-preparer trail-exporter trail-control-plane trail-tool
do
  cd $i
  mv .terraform .terraform.bak
  scripts/terraform-init.sh -backend-config=profiles/pjalybin.init.hcl
  scripts/terraform.sh apply -var-file profiles/pjalybin.tfvars $*
  rm -rf .terraform
  mv .terraform.bak .terraform
  cd ..
done
