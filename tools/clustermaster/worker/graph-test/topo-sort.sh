#!/bin/sh -ex

cat << EOF >> /dev/null
_scenario() {
    THISWORKER = localhost
    THISOTHER = localhost,other
    OTHER = other

    THISWORKER t1: # will be in subgraph (this worker)
    THISWORKER t2: t1 # will be in subgraph (this worker)
    THISOTHER t3: t2 # will be in subgraph (also on this worker)
    THISOTHER t4_cn: t2 # '_cn' suffix sets HadCrossnodeDepends for target to true (and thus makes target not local). But target anyway should be in subgraph
    OTHER t5: t2 # Host other than localhost makes target not local. Anyway it should be in subgraph
}
EOF

# vim: set ts=4 sw=4 et:
