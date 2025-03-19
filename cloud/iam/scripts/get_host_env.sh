#!/usr/bin/env bash

# Identifies the environment by the host fqdn.
function get_host_env {
  case $1 in
    iam-ya-*.cloud.yandex.net)                  echo internal-prod      ;;
    iam-internal-prestable-*.cloud.yandex.net)  echo internal-prestable ;;
    iam-internal-dev-*.cloud.yandex.net)        echo internal-dev       ;;
    *cloud.yandex.net)                          echo prod               ;;
    *cloud-preprod.yandex.net)                  echo preprod            ;;
    *cloud-testing.yandex.net)                  echo testing            ;;
    *green.ya-cloud.net)                        echo green              ;;
    *yandexcloud.co.il)                         echo israel             ;;
    *gcloud.ycinfra.net)                        echo gcloud             ;;
    *cloud-lab.yandex.net)                      echo hwlab              ;;
    *)                                          echo ""                 ;;
  esac
}

# of no args, assume that caller exports the function
if [ "$1" != "" ]; then
  get_host_env $1
fi