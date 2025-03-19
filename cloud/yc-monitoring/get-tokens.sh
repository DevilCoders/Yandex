#!/bin/bash
set -eu

OAUTH_SOLOMON_URL="https://oauth.yandex-team.ru/authorize?response_type=token&client_id=1c0c37b3488143ff8ce570adb66b9dfa"
OAUTH_JUGGLER_URL="https://oauth.yandex-team.ru/authorize?response_type=token&client_id=cd178dcdc31a4ed79f42467f2d89b0d0"
OAUTH_SOLOMON_FILE="token-solomon.sh"
OAUTH_JUGGLER_FILE="token-juggler.sh"
IAM_ISRAEL_FILE="token-solomon-israel.sh"

if [ -f "$OAUTH_JUGGLER_FILE" ]; then
    echo "Juggler OAuth token exists, nothing to do (file: $OAUTH_JUGGLER_FILE)"
else
    echo "OAuth token file doesn't exist: $OAUTH_JUGGLER_FILE"
    echo "Please go to $OAUTH_JUGGLER_URL and paste it here:"
    read token
    echo "export JUGGLER_OAUTH_TOKEN=$token" > "$OAUTH_JUGGLER_FILE"
    echo "OK, saved to: $OAUTH_JUGGLER_FILE"
fi

# OAuth token's ttl is 1 year, so if file exists, assume it works.
if [ -f "$OAUTH_SOLOMON_FILE" ]; then
    echo "OAuth token exists, nothing to do (file: $OAUTH_SOLOMON_FILE)"
else
    echo "OAuth token file doesn't exist: $OAUTH_SOLOMON_FILE"
    echo "Please go to $OAUTH_SOLOMON_URL and paste it here:"
    read token
    echo "export SOLOMON_OAUTH_TOKEN=$token" > "$OAUTH_SOLOMON_FILE"
    echo "OK, saved to: $OAUTH_SOLOMON_FILE"
fi

# ISRAEL IAM Token - reissue, as it is has short ttl
echo -n "Reissuing ISRAEL IAM token ... "
echo "export SOLOMON_IAM_TOKEN=$(ycp --profile israel iam create-token)" > "$IAM_ISRAEL_FILE"
echo "OK, saved to: $IAM_ISRAEL_FILE"

echo
echo "Now you may run 'for f in token-*.sh; do source \$f; done' to load tokens to environment variables."
