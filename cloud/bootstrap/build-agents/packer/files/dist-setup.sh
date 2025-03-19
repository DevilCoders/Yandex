#!/bin/sh

[ $# -gt 0 ] || {
  echo "Usage $0 REPONAME..." >&2
  exit 1
}

DUPCONF="$HOME"/.dupload.conf
TMPFILE="$(mktemp "$DUPCONF".XXXXXXXXXXXX)"
trap 'rm -f "$TMPFILE"' EXIT

cat <<EOF >"$TMPFILE"
package config;
\$default_host = "yc2-devel";
\$preupload{'changes'} = '/usr/share/dupload/gpg-check %1';
EOF

for REPO in "$@"; do
  cat <<EOF >>"$TMPFILE"
\$cfg{'$REPO'} = {
    fqdn => "$REPO.dupload.dist.yandex.ru",
    method => "scpb",
    incoming => "/repo/$REPO/mini-dinstall/incoming/",
# Suppress mail announcements
    dinstall_runs => 1,
#    mailtx => 'bs-upload@yandex-team.ru',
    login => "robot-yc-ci",
};
EOF
done

cat <<EOF >>"$TMPFILE"
# Don't remove the following line.  Perl needs it.
1;
EOF

mv -f "$TMPFILE" "$DUPCONF"
