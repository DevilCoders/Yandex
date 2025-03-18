#!/bin/sh -ex

cat << EOF >> /dev/null
_scenario() {
    HOSTShosts = host00..host49 host00..host49 00..20

    HOSTShosts first
    HOSTShosts second: first [0->1,1->0]
}
EOF

# vim: set ts=4 sw=4 et:
