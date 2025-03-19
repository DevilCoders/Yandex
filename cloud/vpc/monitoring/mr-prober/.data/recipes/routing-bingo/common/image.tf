// FIXME: how to share between clusters
resource "ycp_compute_image" "agent" {
  folder_id = var.folder_id

  name = "${var.prefix}-agent"
  description = "Image for agent built with packer from Mr.Prober images collection"
  uri = "https://storage.yandexcloud.net/yc-vpc-packer-export/mr-prober/agent/fd8a5nv18vv5u40duiig.qcow2"

  os_type = "LINUX"
  min_disk_size = 10
}

resource "ycp_compute_image" "web_server" {
  folder_id = var.folder_id

  name = "${var.prefix}-web-server"
  description = "Image for web server built with packer from Mr.Prober images collection"
  uri = "https://storage.yandexcloud.net/yc-vpc-packer-export/mr-prober/web-server/fd80futih1imolei7857.qcow2"

  os_type = "LINUX"
  min_disk_size = 10
}

