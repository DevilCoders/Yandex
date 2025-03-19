#!/bin/sh

set -ex

cd `dirname $0`/..
terraform apply
