#!/bin/sh -e

$(dirname $0)/zk_check.sh srvr "^Mode: leader$"

# vim: set ts=4 sw=4 et:
