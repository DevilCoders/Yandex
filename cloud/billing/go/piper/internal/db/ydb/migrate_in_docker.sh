#!/bin/bash
ls -1 /code/ydb/*/sql/setup.sql|xargs -L1  /ydb scripting yql -f
