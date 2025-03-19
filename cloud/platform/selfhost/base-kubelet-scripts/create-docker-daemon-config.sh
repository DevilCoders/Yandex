#/bin/sh


echo "creating docker daemon.json"

cat << EOF > /tmp/daemon.json
{
	"log-driver": "fluentd",
		"log-opts": {
			"fluentd-address": "localhost:24224",
			"tag": "app.{{.Name}}.{{.ID}}"
		},
		"bip": "192.168.0.1/24",
		"fixed-cidr": "192.168.0.1/25"
}
EOF

sudo mv /tmp/daemon.json /etc/docker/daemon.json
