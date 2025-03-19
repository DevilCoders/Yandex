#!/usr/bin/env bash
set -eou pipefail

DIR="$(dirname "$0")"
LB_SPEC="$DIR/alb-lb.yaml"
LB_SPEC_BAK="$DIR/alb-lb.bak.yaml"

LBID=$(yq -r '.load_balancer_id' "$LB_SPEC")

echo "Fetch actual version of the load balacer resource..."
ycp --profile=prod platform alb load-balancer get "$LBID" > "$LB_SPEC_BAK"

echo "Updating labels"
yq '{labels, update_mask: {paths: ["labels"]}}' "$LB_SPEC" | \
  ycp --profile=prod platform alb load-balancer update "$LBID" -r-
