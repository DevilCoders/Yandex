#!/bin/sh -ex

cat << EOF >> /dev/null
_scenario() {
    HOSTShosts = host000..host499 host000..host499

    HOSTShosts first
    HOSTShosts second
}
EOF

# vim: set ts=4 sw=4 et:
