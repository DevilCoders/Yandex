create_topic() {
    local _topic=$1
    local _parts=$2

    ${LB_UTIL} schema create topic \
        ${_topic} \
        --abc-service dbaas \
        --retention-period-sec 129600 \
        -p ${_parts} \
        --responsible 'mdb@' \
        -y
}

create_consumer() {
    local _path=$1

    ${LB_UTIL} schema create consumer \
        ${_path} \
        -y
}

allow_consumer_read_topic() {
    local _consumer=$1
    local _topic=$2

    local _basepath=$(dirname ${_topic})
    local _path="${_basepath}/${_consumer}"

    ${LB_UTIL} schema create read-rule \
        -y --topic ${_topic} \
        --all-original \
        --consumer ${_path}
}

allow_datatransfer_read_topic() {
    local _sa_account=$1
    local _topic=$2

    ${LB_UTIL} permissions grant \
        -y --path ${_topic} \
        --subject ${_sa_account} \
         --permissions ReadTopic
}

allow_datatransfer_read_topic_as_consumer() {
    local _sa_account=$1
    local _topic=$2
    local _consumer=$3

    local _consumer_path="$(dirname ${_topic})/${_consumer}"

    ${LB_UTIL} permissions grant \
        -y --path ${_consumer_path} \
        --subject ${_sa_account} \
        --permissions ReadAsConsumer
}

allow_producer_write_topic() {
    local _producer_sa=$1
    local _topic=$2

    ${LB_UTIL} permissions grant \
        -y --path ${_topic} \
        --subject "${_producer_sa}" \
        --permissions WriteTopic
}

