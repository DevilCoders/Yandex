#!/bin/sh -ex

cat << EOF >> /dev/null
_scenario() {
    PERBOX   = worker1..worker4
    GATHER   = main worker1..worker4

    PERBOX   join
    GATHER   copy_from_workers
}
EOF

# vim: set ts=4 sw=4 et:
