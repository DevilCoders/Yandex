data "template_file" "infra-configs-no-push-client" {
  template = "${file("${path.module}/files/infra-configs-no-push-client.tpl")}"

  vars = {
    fluent_conf = "${file("${path.module}/../../common/files/fluent/fluent.conf")}"
    fluent_containers_input_conf = "${file("${path.module}/../../common/files/fluent/config.d/containers.input.conf")}"
    fluent_system_input_conf = "${file("${path.module}/../../common/files/fluent/config.d/system.input.conf")}"
    fluent_monitoring_conf = "${file("${path.module}/../../common/files/fluent/config.d/monitoring.conf")}"
    fluent_output_conf = "${file("${path.module}/../../common/files/fluent/config.d/output.conf")}"
    solomon_agent_conf = "${file(local.solomon_agent_conf_path)}"
    solomon_agent_als_prom_conf = "${file(local.solomon_als_prometheus_config_path)}"
    solomon_agent_envoy_prom_conf = "${file(local.solomon_envoy_prometheus_config_path)}"
    solomon_agent_gateway_prom_conf = "${file(local.solomon_gateway_prometheus_config_path)}"
    solomon_agent_kubelet_prom_conf = "${file(local.solomon_kubelet_prometheus_config_path)}"
    solomon_agent_system_plugin_conf = "${file("${path.module}/../../common/files/solomon-agent/system-plugin.conf")}"
    juggler_bundle_manifest = "${local.juggler_bundle_manifest}"
    platform_http_check_json = "${local.platform_http_check_json_content}"
    juggler_client_config = "${file("${path.module}/files/juggler-client/juggler-client.conf")}"
  }
}

data "template_file" "infra-pod-spec-no-push-client" {
  template = "${file("${path.module}/files/infra-pod-spec-no-push-client.yaml.tpl")}"

  vars = {
    infra_config_digest = "${sha256(data.template_file.infra-configs-no-push-client.rendered)}"
    metadata_version = "${var.metadata_image_version}"
    juggler_version = "${var.juggler_client_image_version}"
    solomon_version = "${var.solomon_agent_image_version}"
    logcleaner_version = "${var.logcleaner_image_version}"
    logcleaner_files_to_keep = "${var.logcleaner_files_to_keep}"
    fluent_version = "${var.fluent_image_version}"
  }
}
