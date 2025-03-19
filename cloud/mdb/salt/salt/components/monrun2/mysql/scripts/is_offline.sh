#!/bin/bash
exec mysql -Nse 'select @@offline_mode' mysql 2>/dev/null | grep -qw 1
