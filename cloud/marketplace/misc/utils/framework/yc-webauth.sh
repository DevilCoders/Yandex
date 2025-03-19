source /opt/yc-marketplace/yc-env.sh
source /opt/yc-marketplace/yc-welcome-msg.sh

gen_pwd() {
    local admin_pass=`curl http://${AWS_METADATA_IP}/latest/meta-data/instance-id`

    if [ -z ${admin_pass} ];
    then
        host ${GCLOUD_METADATA_HOST} > /dev/null
        if [[ $? == 0 ]];
        then
            admin_pass=`curl "http://${GCLOUD_METADATA_HOST}/computeMetadata/v1/instance/id" -H "Metadata-Flavor: Google"`
        else
            admin_pass=`curl "http://${AWS_METADATA_IP}/computeMetadata/v1/instance/id" -H "Host: ${GCLOUD_METADATA_HOST}" -H "Metadata-Flavor: Google"`
        fi
    fi

    if [ -z ${admin_pass} ];
    then
        print_tty "#### Something wents wrong (generate webauth password)" > /dev/null
    fi

    echo ${admin_pass}
}
