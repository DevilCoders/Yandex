
data "template_file" "infra-configs-no-fluentd" {
  template = "${file("${path.module}/files/specs/no-fluentd/infra-configs-no-fluentd.tpl")}"

  vars = {
    push_client_conf = "${file(local.push_client_conf_path)}"
    juggler_bundle_manifest = "${local.juggler_bundle_manifest}"
    platform_http_check_json = "${local.platform_http_check_json_content}"
    juggler_client_config = "${file("${path.module}/files/juggler-client/juggler-client.conf")}"
    metrics_agent_config = "${data.template_file.metrics_agent_config.rendered}"
  }
}

data "template_file" "infra-pod-spec-no-fluentd" {
  template = "${file("${path.module}/files/specs/no-fluentd/infra-pod-spec-no-fluentd.yaml.tpl")}"

  vars = {
    infra_config_digest = "${sha256(data.template_file.infra-configs-no-fluentd.rendered)}"
    metadata_version = "${var.metadata_image_version}"
    juggler_version = "${var.juggler_client_image_version}"
    push_client_version = "${var.push-client_image_version}"
    metrics_agent_memory_limit = "${var.metrics_agent_memory_limit}"
  }
}
