#!/usr/bin/env bash
set -eo pipefail

CLOUD=b1g3clmedscm7uivcn4a

for f in $(ycp --profile=prod resource-manager folder list --cloud-id $CLOUD | yq -r  .[].id); do
  ycp --profile=prod microcosm instance-group list --folder-id "$f" \
  | yq -c '.[]|[.instance_template.boot_disk_spec.disk_spec.image_id, .name]'
done | sort
