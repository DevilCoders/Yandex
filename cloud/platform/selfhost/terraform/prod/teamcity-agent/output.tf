output "instances_fqdn_ipv6" {
  value = [
    formatlist("%s.ycp.cloud.yandex.net %s", yandex_compute_instance.agent.*.name, yandex_compute_instance.agent.*.network_interface.0.ipv6_address)
  ]
}
