#!/bin/bash

cd `dirname $0`

skm decrypt-bundle --bundle skm-encrypted-keys.yaml
