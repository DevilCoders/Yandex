#!/bin/bash

set -ev

MYSQL_ROOT_PASS=ZkbEsVNb2rXKT

/root/bitrix-env.sh -s -p -H "defaultServer" -M $MYSQL_ROOT_PASS
