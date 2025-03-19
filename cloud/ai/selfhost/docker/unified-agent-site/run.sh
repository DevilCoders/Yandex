#!/usr/bin/env bash

set -e

if test -z "$BUILD_ENV"
then
  printf "Environment doesn't send" >&2
  exit 1;
fi

env="$BUILD_ENV"

case $env in
  "preprod")
    config="unified-agent-preprod.yaml"
    ;;
  "staging")
    config="unified-agent-staging.yaml"
    ;;
  "prod")
    config="unified-agent-prod.yaml"
    ;;
  *)
    printf "Environment is not valid: received - '%s', should be in [preprod, staging, prod]\n" "$BUILDENV" >&2
    exit 1
    ;;
esac

configpath="/etc/yandex/unified_agent/$config"

/usr/bin/unified_agent --log-priority RESOURCES --config "$configpath"
