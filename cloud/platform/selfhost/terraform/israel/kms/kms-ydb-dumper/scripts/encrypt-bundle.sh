#!/bin/sh

YC_TOKEN=`ycp --profile israel --format text iam v1 create-token` skm encrypt-bundle --config "$(dirname $0)/../files/skm/skm.yaml" --bundle "$(dirname $0)/../files/skm/bundle.yaml"
