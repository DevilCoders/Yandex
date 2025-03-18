#!/bin/bash
tar -zxf templater.tar.gz
tar -zxf config.tar.gz
export PYTHONPATH=${PYTHONPATH}:$(pwd)/lib/python2.7
env > minimal_env
bin/templater --log-level debug -l templater.log --ignore-errors
actual_log_dir="/db/www/logs/${BSCONFIG_INAME}"
if [ ! -d "$actual_log_dir" ]; then
    mkdir "$actual_log_dir"
fi
ln --symbolic --force --no-dereference "$actual_log_dir" logs
exec ./"instancectl" install -c "instancectl.conf"
