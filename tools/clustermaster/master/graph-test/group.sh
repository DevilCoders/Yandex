#!/bin/sh -ex

cat << EOF >> /dev/null
_scenario() {
    QQQ = !hostcfg:h
    WWW = !hostcfg:W
    QQQ first:
    WWW second: first %
}
EOF

# vim: set ts=4 sw=4 et:
