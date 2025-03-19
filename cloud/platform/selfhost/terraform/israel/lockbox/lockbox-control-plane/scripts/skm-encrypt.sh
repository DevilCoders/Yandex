#!/bin/sh

YC_TOKEN=`ycp --profile israel --format text iam v1 create-token` skm encrypt-md --format raw --config "$(dirname $0)/../files/skm/skm.yaml" > "$(dirname $0)/../files/skm/bundle.yaml"
