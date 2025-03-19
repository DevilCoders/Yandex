#!/bin/sh

YC_TOKEN=`ycp --profile israel --format text iam v1 create-token` skm edit-bundle "$(dirname $0)/../files/skm/bundle.yaml"
