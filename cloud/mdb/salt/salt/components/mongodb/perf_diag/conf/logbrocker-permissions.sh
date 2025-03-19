#!/bin/bash -xe

_lb="logbroker -s logbroker"
_robot="robot-lf-dyn-push@staff"

enable_consumer() {
    local consumer="$(dirname $1)/mongodb-consumer"

    ${_lb} schema create read-rule \
        -y --topic $1 \
        --all-original \
        --consumer $(dirname $1)/mongodb-consumer
}

allow_read_topic_robot() {
    local consumer="$(dirname $1)/mongodb-consumer"
    ${_lb} permissions grant \
        -y --path $1 \
        --subject ${_robot} \
         --permissions ReadTopic
}

allow_robot_as_consumer() {
    local consumer="$(dirname $1)/mongodb-consumer"
    ${_lb} permissions grant \
        -y --path ${consumer} \
        --subject ${_robot} \
        --permissions ReadAsConsumer
}

#for env in "porto/prod" "porto/test" "compute/preprod" "compute/prod"
for env in "porto/prod"
do
    for topic in mongodb_profiler
    do
        topic_path=mdb/${env}/perf_diag/${topic}
        enable_consumer $topic_path
        allow_read_topic_robot $topic_path
        allow_robot_as_consumer $topic_path
    done
done

