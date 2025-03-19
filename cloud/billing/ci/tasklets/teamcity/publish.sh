#!/bin/bash
set -e

cd `dirname .`
ya make -r .
./teamcity sb-upload --sb-schema
