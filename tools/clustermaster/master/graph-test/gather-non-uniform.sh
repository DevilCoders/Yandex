#!/bin/sh -ex

cat << EOF >> /dev/null
_scenario() {
    WWW = !hostcfg:W
    QQQ = !hostcfg:h !hostcfg:M
    WWW first:
    QQQ second: first
}
EOF

# vim: set ts=4 sw=4 et:
