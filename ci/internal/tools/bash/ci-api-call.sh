#!/bin/bash

export TARGET_HOST="ci-api.in.yandex-team.ru:4221"
export TVM_SERVICE_TICKET=$(ya tool tvmknife get_service_ticket sshkey --src 2018912 --dst 2018912 | sed  's/:/\\:/g')

PAYLOAD='release_process_id { dir: "mail/payments-sdk-backend" id: "payments-sdk-backend-release" } limit: 10'
grpc_cli call --metadata="x-request-id:$USER-cli-`openssl rand -hex 16`:x-ya-service-ticket:${TVM_SERVICE_TICKET}" ${TARGET_HOST} frontend.TimelineService.GetTimeline "${PAYLOAD}"
