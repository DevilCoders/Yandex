#!/usr/bin/env bash
# Given a subpackage and the containing package, figures out which packages
# need to be passed to `go test -coverpkg`:  this includes all of the
# subpackage's dependencies within the containing package, as well as the
# subpackage itself.
REPO=a.yandex-team.ru/cloud/compute/snapshot
DEPENDENCIES="$(go list -f $'{{range $f := .Deps}}{{$f}}\n{{end}}' ${1} | grep ${REPO} | grep -v ${REPO}/vendor)"
TESTDEPENDENCIES="$(go list -f $'{{range $f := .TestImports}}{{$f}}\n{{end}}' ${1} | grep ${REPO} | grep -v ${REPO}/vendor)"
echo "${1} ${DEPENDENCIES} ${TESTDEPENDENCIES}" | xargs echo -n | tr " " "\n" | sort | uniq | tr "\n" "," | sed 's/,$//'
