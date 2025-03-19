resource "ycp_compute_image" "dns_proxy" {
  description   = "dns-proxy.qcow2"
  folder_id     = var.folder_id
  name          = "dns-proxy"
  uri           = lookup(var.image_urls_map, "DNS_PROXY", "https://storage.yandexcloud.net/yc-vpc-packer-export/dns-proxy/fd8fka10oob8ao39brsn.qcow2")
  labels        = {}
  os_type       = "LINUX"
  min_disk_size = 20
}

resource "ycp_compute_placement_group" "dns_proxy" {
  folder_id = var.folder_id
  name      = "dns-proxy"

  spread_placement_strategy {
    best_effort            = false
    max_instances_per_node = 1
  }
}
