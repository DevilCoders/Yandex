locals {
  hostname = "${ var.hostname != "default_hostname" ? "${var.hostname}" : "" }"

  solomon_agent_conf_path = "${ var.solomon_agent_conf_path != "_intentionally_empty_file" ?
     var.solomon_agent_conf_path :
     "${path.module}/../../common-gateway/files/solomon-agent/agent.conf" }"

  solomon_als_prometheus_config_path = "${ var.solomon_prometheus_native ?
    "${path.module}/../../common/files/solomon-agent/native-prometheus-module/als-prometheus.conf" :
    "${path.module}/../../common/files/solomon-agent/als-prometheus.conf" }"

  solomon_envoy_prometheus_config_path = "${ var.solomon_prometheus_native ?
    "${path.module}/../../common/files/solomon-agent/native-prometheus-module/envoy-prometheus.conf" :
    "${path.module}/../../common/files/solomon-agent/envoy-prometheus.conf" }"

  solomon_gateway_prometheus_config_path = "${ var.solomon_prometheus_native ?
    "${path.module}/../../common/files/solomon-agent/native-prometheus-module/gateway-prometheus.conf" :
    "${path.module}/../../common/files/solomon-agent/gateway-prometheus.conf" }"

  solomon_kubelet_prometheus_config_path = "${ var.solomon_prometheus_native ?
    "${path.module}/../../common/files/solomon-agent/native-prometheus-module/kubelet-prometheus.conf" :
    "${path.module}/../../common/files/solomon-agent/kubelet-prometheus.conf" }"

  push_client_conf_path = "${ var.push_client_conf_path != "_intentionally_empty_file" ?
    var.push_client_conf_path :
    "${path.module}/../../common-gateway/files/push-client/push-client.yaml" }"

  existing_common_http_check_path = "${path.module}/../../common/files/juggler/platform-http-${var.juggler_check_name}.json"
  # To avoid calling stat() for non existing file path, precalculate target path that will always exists.
  platform_http_check_path_hack = "${ var.platform_http_check_path != "_intentionally_empty_file" ? var.platform_http_check_path : local.existing_common_http_check_path}"
  # Now we can use file() to get the content.
  platform_http_check_json_content = "${file(local.platform_http_check_path_hack)}"

  # To support explicitly specified manifest file (mostly for backward compatibility).
  juggler_bundle_manifest = "${ var.juggler_bundle_manifest_path != "_intentionally_empty_file" ? file(var.juggler_bundle_manifest_path) : data.template_file.infra-manifest-json.rendered}"

  juggler_check_names = length(var.juggler_check_names) > 0 ? var.juggler_check_names : [ var.juggler_check_name ]
}

data "template_file" "infra-manifest-json" {
  template = "${file("${path.module}/../../common/files/juggler/MANIFEST.tpl.json")}"

  vars = {
    juggler_checks = templatefile("${path.module}/files/juggler-http-checks.tpl.json", {juggler_check_names = "${local.juggler_check_names}" })
  }
}
