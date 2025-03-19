#!/bin/bash
set -e
mkdir -p /backup/
is_slave || exit 0
tar -cvf - /opt/CVSROOT/ | zstd -6 - -o /backup/cvs-$(date "+%Y-%m-%d-%H-%M-%S").tar.zst
find /backup/ -name "cvs-.*tar.zst" -mtime +7 -type d -exec rm -rv {} \+
