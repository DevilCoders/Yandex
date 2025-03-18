#!/bin/sh -ex

cat << EOF >> /dev/null
_scenario() {
    SCATTER  = main worker1..worker4
    PERBOX   = worker1..worker4

    SCATTER  copy_to_workers
    PERBOX   split_8
}
EOF

# vim: set ts=4 sw=4 et:
