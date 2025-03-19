provider "ycp" {
  prod      = false
  folder_id = "${var.yc_folder}"
  zone      = "${var.yc_zone}"
  ycp_profile = "trail-preprod"
}

data "template_file" "application_yaml" {
  template = "${file("${path.module}/files/trail/application.tpl.yaml")}"
  count    = "${var.yc_instance_group_size}"
  vars = {
      ydb_tablespace = "${var.ydb_tablespace}"
  }
}

data "template_file" "configs" {
  template = "${file("${path.module}/files/trail/configs.tpl")}"
  count    = "${var.yc_instance_group_size}"

  vars = {
    application_yaml = "${element(data.template_file.application_yaml.*.rendered, count.index)}"
  }
}

data "template_file" "infra_configs" {
  template = "${file("${path.module}/files/trail/infra-configs.tpl")}"

  vars = {
    solomon_agent_conf  = "${file("${path.module}/../common/solomon-agent.conf")}"
    push_client_conf    = "${file("${path.module}/files/push-client/push-client.yaml")}"
    metrics_agent_conf  = "${file("${path.module}/../common/metricsagent.yaml")}"
  }
}

data "template_file" "podmanifest" {
  template = "${file("${path.module}/files/trail/podmanifest.tpl.yaml")}"
  count    = "${var.yc_instance_group_size}"

  vars = {
    config_digest          = "${sha256(element(data.template_file.configs.*.rendered, count.index))}"
    infra_config_digest    = "${sha256(data.template_file.infra_configs.rendered)}"
    solomon_version        = "${var.solomon_agent_image_version}"
    application_version    = "${var.application_version}"
    app_log_level          = "${var.app_log_level}"
    application_container_max_memory = "${var.application_container_max_memory}"
    application_jvm_xmx    = "${var.application_jvm_xmx}"
  }
}

resource "ycp_compute_disk" "secondary-disks" {
  count   = "${var.yc_instance_group_size}"
  name    = "${var.name_prefix}-${lookup(var.yc_zone_suffix, element(var.yc_zones, count.index))}-${floor(count.index / length(var.yc_zones)) + count.index % length(var.yc_zones) - index(var.yc_zones, element(var.yc_zones, count.index)) + 1}-secondary-disk"
  type_id = "${var.secondary_disk_type}"
  size    = "${var.secondary_disk_size}"
  zone_id = "${element(var.yc_zones, count.index % 3)}"
}

module "control-plane-instance-group" {
  source          = "../../../modules/kubelet_instance_group_ycp"
  name_prefix     = "${var.name_prefix}"
  hostname_prefix = "${var.hostname_prefix}"
  hostname_suffix = "${var.hostname_suffix}"
  role_name       = "trail-control"
  osquery_tag     = "ycloud-svc-cloud-trail"

  zones = "${var.yc_zones}"

  instance_group_size = "${var.yc_instance_group_size}"

  cores_per_instance  = "${var.instance_cores}"
  core_fraction_per_instance = "${var.instance_core_fraction}"
  memory_per_instance = "${var.instance_memory}"
  disk_per_instance   = "${var.instance_disk_size}"
  disk_type           = "${var.instance_disk_type}"
  image_id            = "${var.image_id}"

  configs              = "${data.template_file.configs.*.rendered}"
  infra-configs        = "${data.template_file.infra_configs.rendered}"
  podmanifest          = "${data.template_file.podmanifest.*.rendered}"
  skip_update_ssh_keys = true

  metadata = {
    skm = "${file("${path.module}/files/skm/skm.yaml")}"
    k8s-runtime-bootstrap-yaml = "${file("${path.module}/../common/bootstrap.yaml")}"
    application-config = templatefile("${path.module}/files/trail/application.tpl.yaml", {
      ydb_tablespace = "${var.ydb_tablespace}"
    })
    solomonagent-config = "${file("${path.module}/../common/solomon-agent.conf")}"
    metricsagent-config = "${file("${path.module}/../common/metricsagent.yaml")}"
    push-client-config = "${file("${path.module}/files/push-client/push-client.yaml")}"
    jaeger-agent-config = "${file("${path.module}/../common/jaeger-agent.yaml")}"
    solomon-agent-pod = templatefile("${path.module}/../common/solomon-agent-pod.tpl.yaml", {
      solomon_version = "${var.solomon_agent_image_version}"
    })
    application-pod = templatefile("${path.module}/files/trail/podmanifest.tpl.yaml", {
      application_version    = "${var.application_version}"
      app_log_level          = "${var.app_log_level}"
      application_container_max_memory = "${var.application_container_max_memory}"
      application_jvm_xmx    = "${var.application_jvm_xmx}"
    })
    nsdomain = "${var.hostname_suffix}"
    user-data = "${file("${path.module}/../common/cloud-init.yaml")}"
    enable-oslogin = true
  }

  labels = {
    layer   = "paas"
    abc_svc = "yccloudtrail"
    env     = "pre-prod"
  }

  subnets        = "${var.subnets}"
  ipv4_addresses = "${var.ipv4_addresses}"
  ipv6_addresses = "${var.ipv6_addresses}"
  underlay       = false

  service_account_id = "${var.service_account_id}"
  host_group         = "${var.host_group}"
  instance_platform_id = "${var.instance_platform_id}"
  security_group_ids   = "${var.security_group_ids}"

  need_secondary_disk        = true
  secondary_disk_auto_delete = false
  secondary_disk_device_name = "log"
  secondary_disks            = ycp_compute_disk.secondary-disks
}
