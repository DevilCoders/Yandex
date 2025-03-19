#!/bin/bash

# use as .source for prepare environment for start snapshot

export PS1='(arcadia-go) \W:$ '
cd /vagrant/arcadia/src/a.yandex-team.ru/cloud/compute/snapshot/cmd/yc-snapshot
export GOROOT="$(ya tool go --print-toolchain-path)"
export PATH="$GOROOT/bin:$PATH"
export GOPATH=/vagrant/arcadia/
ya make --add-result=go --replace-result
