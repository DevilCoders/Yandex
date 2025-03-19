#!/usr/bin/env bash
set -e
set -x

# 5.7
if [ -d percona-xtradb-cluster-server-5.7 ]; then
    rm -f percona-xtradb-cluster-server-5.7/DEBIAN/postinst
    rm -f percona-xtradb-cluster-server-5.7/DEBIAN/preinst
    rm -rf percona-xtradb-cluster-server-5.7/etc/mysql
    sed -i -e '/\/etc\/mysql\/.*/d' percona-xtradb-cluster-server-5.7/DEBIAN/conffiles
fi

if [ -d percona-xtradb-cluster-common-5.7 ]; then
    rm -f percona-xtradb-cluster-common-5.7/DEBIAN/postinst
    rm -f percona-xtradb-cluster-common-5.7/DEBIAN/preinst
    rm -rf percona-xtradb-cluster-common-5.7/etc/mysql
    sed -i -e '/\/etc\/mysql\/.*/d' percona-xtradb-cluster-common-5.7/DEBIAN/conffiles
fi

if [ -d percona-xtradb-cluster-client-5.7 ]; then
    # nothing
    true
fi

# 8.0
if [ -d percona-server-server ]; then
    rm -f percona-server-server/DEBIAN/postinst
    rm -f percona-server-server/DEBIAN/preinst
    rm -rf percona-server-server/etc/mysql
    sed -i -e '/\/etc\/mysql\/.*/d' percona-server-server/DEBIAN/conffiles
    sed -i -e 's/^Breaks:\s*/Breaks: percona-xtradb-cluster-server-5.7, /' percona-server-server/DEBIAN/control
    sed -i -e 's/^Replaces:\s*/Replaces: percona-xtradb-cluster-server-5.7, /' percona-server-server/DEBIAN/control
fi

if [ -d percona-server-common ]; then
    rm -f percona-server-common/DEBIAN/postinst
    rm -rf percona-server-common/etc/mysql
    sed -i -e '/\/etc\/mysql\/.*/d' percona-server-common/DEBIAN/conffiles
    sed -i -e 's/^Breaks:\s*/Breaks: percona-xtradb-cluster-common-5.7, /' percona-server-common/DEBIAN/control
    sed -i -e 's/^Replaces:\s*/Replaces: percona-xtradb-cluster-common-5.7, /' percona-server-common/DEBIAN/control
fi

if [ -d percona-server-client ]; then
    sed -i -e 's/^Breaks:\s*/Breaks: percona-xtradb-cluster-client-5.7, /' percona-server-client/DEBIAN/control
    sed -i -e 's/^Replaces:\s*/Replaces: percona-xtradb-cluster-client-5.7, /' percona-server-client/DEBIAN/control
fi
