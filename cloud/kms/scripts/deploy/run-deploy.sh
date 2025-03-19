#!/bin/bash

if [ ! -x `dirname $0`/terraform/deploy ]; then
  echo "Compiling..."
  ya make -r `dirname $0`/terraform
fi
exec `dirname $0`/terraform/deploy $*
