#/bin/sh

echo "creating kubelet.service"

manifest_url="http://169.254.169.254/computeMetadata/v1/instance/attributes/podmanifest"
metadata_image="registry.yandex.net/cloud/kozhapenko/api-metadata:2497adf22"

cat << EOF > /tmp/kubelet.service
[Unit]
Description=Yandex.Cloud API Base Image Kubelet Service
After=docker.service metadata.service
Requires=docker.service
Wants=metadata.service fluent.service

[Service]
ExecStart=/usr/bin/kubelet \
--address=127.0.0.1 \
--pod-manifest-path=/etc/kubernetes/manifests \
--manifest-url ${manifest_url} \
--manifest-url-header 'Metadata-Flavor:Google' \
--allow-privileged=true
Restart=always

[Install]
WantedBy=multi-user.target
EOF

sudo mv /tmp/kubelet.service /lib/systemd/system/kubelet.service
