#!/bin/sh

DIR="${0%/*}"

export GNUPGHOME="$HOME/.gnupg"
mkdir -p "$GNUPGHOME"
chmod 700  "$GNUPGHOME"

gpg --import "$DIR/robot-yc-ci@yandex-team.ru.seckey"

if gpg --export-secret-keys | gpg2 --import -; then
  :
elif [ $? -eq 2 ]; then # Keys exist
  :
else
  exit $?
fi
