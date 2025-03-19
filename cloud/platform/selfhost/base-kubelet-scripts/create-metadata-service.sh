#/bin/sh

echo "creating metadata.service"

metadata_image="registry.yandex.net/cloud/api/metadata:48a9aebb1"

cat << EOF > /tmp/metadata.service
[Unit]
Description=Yandex.Cloud API Base Image Metadata Reader

After=docker.service fluent.service
Before=kubelet.service
Requires=docker.service fluent.service

[Service]
Type=oneshot
ExecStartPre=/bin/sleep 60
ExecStart=/usr/bin/docker \
--config /var/lib/kubelet run \
--network host \
-v /etc:/etc \
${metadata_image}

[Install]
WantedBy=multi-user.target
EOF

sudo mv /tmp/metadata.service /lib/systemd/system/metadata.service
