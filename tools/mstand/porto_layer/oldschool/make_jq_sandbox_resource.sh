#!/bin/sh

wget https://github.com/stedolan/jq/releases/download/jq-1.5/jq-1.5.tar.gz -O jq.tgz
ya upload --ttl 730 jq.tgz
rm jq.tgz
