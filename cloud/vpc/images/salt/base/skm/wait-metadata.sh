#!/bin/bash
until curl --fail -sS 'http://169.254.169.254/computeMetadata/v1/instance/attributes' -H 'Metadata-Flavor: Google' >/dev/null
do
  echo 'Retrying metadata service'
  sleep 5
done
