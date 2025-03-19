#/bin/sh


echo "creating fluent service"

metadata_url="http://169.254.169.254/computeMetadata/v1/instance/attributes/docker-config"
metadata_header="Metadata-Flavor:Google"

cat <<EOF > /tmp/fluent.service
[Unit]
Description=Yandex.Cloud API Base Image Fluentd

After=docker.service
Before=kubelet.service metadata.service
Requires=docker.service

[Service]
ExecStartPre=/usr/bin/curl \
${metadata_url} \
-H ${metadata_header} \
-o /var/lib/kubelet/config.json
ExecStart=/usr/bin/docker --config /var/lib/kubelet run -p 24224:24224 -p 24224:24224/udp -v /var/log/fluent:/fluentd/log registry.yandex.net/cloud/api/fluent:1659bd16f
Restart=always

[Install]
WantedBy=multi-user.target
EOF

sudo mv /tmp/fluent.service /lib/systemd/system/fluent.service

echo "creating fluent config"

cat <<EOF > /tmp/fluent.conf
<source>
  @type  forward
  @id    input1
  @label @mainstream
  port  24224
</source>

<filter **>
  @type stdout
</filter>

<label @mainstream>
  <match app.*.*>
    @type rewrite_tag_filter
    <rule>
      key source
      pattern /^(\w+)$/
      tag $1.${tag}
    </rule>
  </match>

  <match *.app.*.*>
    @type rewrite_tag_filter
    <rule>
      key container_name
# /k8s_api-gateway_api-static-ef38mjftmv6oqbd9m5qg_default_d4339ef7b80693ff45d2793d6c5d6154_0
      pattern /^\/k8s_(.+)_.*$/
      tag yc.$1.${tag}
    </rule>
    <rule>
      key container_name
      pattern /^\/(.+)$/
      tag system.${tag}
    </rule>
  </match>

  <match system.**>
    @type file
    @id   output_system1
    path         /var/log/fluent/system.*.log
    symlink_path /var/log/fluent/system.log
    append       true
    time_slice_format %Y%m%d
    time_slice_wait   1m
    time_format       %Y%m%dT%H%M%S%z
  </match>

  <filter yc.*.stdout.app.*.*>
    @type record_modifier
    whitelist_keys log
  </filter>

  <match yc.*.stdout.app.*.*>
    @type file
    @id   yc_api_acesslog_output1
    path  /var/log/fluent/yc_api_accesslog.%Y%m%dT%H
    symlink_path /var/log/fluent/access.log
    compress gzip
    <buffer time>
      @type file
      path /var/log/fluent/buffers
      timekey 1h
      timekey_use_utc true
      timekey_wait 10m
    </buffer>
    <format>
      @type single_value
      message_key log
    </format>
  </match>

  <match yc.*.stderr.app.*.*>
    @type file
    @id   yc_api_stderr_output1
    path         /var/log/fluent/yc_api_stderr.*.log
    symlink_path /var/log/fluent/yc_api_stderr.log
    append       true
    time_slice_format %Y%m%d
    time_slice_wait   10m
    time_format       %Y%m%dT%H%M%S%z
  </match>

  <match **>
    @type file
    @id   rest_output1
    path         /var/log/fluent/rest.*.log
    symlink_path /var/log/fluent/rest.log
    append       true
    time_slice_format %Y%m%d
    time_slice_wait   10m
    time_format       %Y%m%dT%H%M%S%z
  </match>
</label>
EOF

sudo mkdir -p /etc/fluent
sudo mv /tmp/fluent.conf /etc/fluent/fluent.conf
