#!/bin/bash

# Try getting token from selfdns-token attribute.
token=`curl -H "Metadata-flavor: Google" -s -f "http://169.254.169.254/computeMetadata/v1/instance/attributes/selfdns-token"`
status=$?
if [ $status -ne 0 ]; then
    echo "Attribute selfdns-token not present"
    exit 0
fi

grep -v "token =" /etc/yandex/selfdns-client/default.conf > /tmp/selfdns-default.conf
echo "token = "$token >> /tmp/selfdns-default.conf
mv /tmp/selfdns-default.conf /etc/yandex/selfdns-client/default.conf
