data "template_file" "metadata" {
  template = file("../../scripts/metadata.yaml")

  vars = {
    self_dns_api = var.self_dns_api_host
    domain       = var.domain
    service_name = var.service_name
    per_boot_sh = replace(templatefile("../../scripts/per_boot.sh", {
      env_name            = var.env_name
      packet_name         = var.packet_name
      service_name        = var.service_name
      packet_version      = var.packet_version
      lockbox_api_host    = var.lockbox_api_host
      selfdns_secret      = var.selfdns_secret
      sa_consumer_secret  = var.sa_consumer_secret
      sa_consumer_version = var.sa_consumer_version
      searchmap           = var.searchmap
      zk_config           = var.zk_config
    }), "\n", "\\n")
    update_monitoring_sh = replace(templatefile("../../scripts/update_monitoring.sh", {
      packet_version      = var.solomon_agent_version
      agent_conf          = replace(templatefile("../../envs/configs/solomon/agent.conf", {}), "\n", "\\n")
      system_conf         = replace(templatefile("../../envs/configs/solomon/system.conf", {}), "\n", "\\n")
      yc_search_stat_conf = replace(templatefile("../../envs/configs/solomon/yc-search-stat.conf", {}), "\n", "\\n")
      systemd_cfg         = replace(templatefile("../../envs/configs/solomon/solomon-agent.service", {}), "\n", "\\n")
    }), "\n", "\\n")
  }
}

terraform {
  required_version = ">= 0.14.7, < 0.15.0"
}
