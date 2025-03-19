#!/bin/sh

YC_TOKEN=`ycp --profile hw-blue-lab-lockbox --format text iam v1 create-token` skm edit-bundle "$(dirname $0)/../files/skm/bundle.yaml"
