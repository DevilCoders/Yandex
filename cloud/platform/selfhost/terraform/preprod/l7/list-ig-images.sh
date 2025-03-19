#!/usr/bin/env bash
set -eo pipefail

CLOUD=aoe0oie417gs45lue0h4

for f in $(ycp --profile=preprod resource-manager folder list --cloud-id $CLOUD | yq -r  .[].id); do
  for ig in $(ycp --profile=preprod microcosm instance-group list --folder-id "$f" | yq -r .[].id); do
    ycp --profile=preprod microcosm instance-group get $ig \
    | yq -c '[.instance_template.boot_disk_spec.disk_spec.image_id, .name]'
  done
done | sort
