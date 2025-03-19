#!/bin/bash

# log_tree stack overflow workaround
ulimit -s 16384
TMPDIR=/data/tmp/ /usr/bin/cvs -d /opt/CVSROOT/ server
