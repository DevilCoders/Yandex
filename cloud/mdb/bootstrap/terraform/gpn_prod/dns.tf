resource "ycp_compute_instance" "mdb-dns_hosts" {
  count = 2

  name                      = format(var.name_format, "mdb-dns", count.index + 1, var.zone_suffix)
  zone_id                   = var.zone_id
  platform_id               = "standard-v2"
  fqdn                      = format(var.fqdn_format, "mdb-dns", count.index + 1, var.zone_suffix, var.fqdn_suffix)
  allow_stopping_for_update = true

  resources {
    core_fraction = 50
    cores         = 2
    memory        = 4
  }

  boot_disk {
    disk_spec {
      image_id = "d8ocvca3ul3o1n0lqs13"
      size     = 30
    }
  }

  network_interface {
    subnet_id = data.ycp_vpc_subnet.mdb-control-plane-nets-ru-gpn-spb99.subnet_id
    primary_v6_address {}
  }

  provisioner "file" {
    source      = "../scripts/provision_host.py"
    destination = "/root/provision_host.py"
    connection {
      type = "ssh"
      user = "root"
      host = self.network_interface[0].primary_v6_address[0].address
    }
  }
  provisioner "remote-exec" {
    inline = [
      "chmod +x /root/provision_host.py",
      "/root/provision_host.py --fqdn=${self.fqdn} --token=${var.mdb_admin_token} --deploy=${var.mdb_deploy_api}"
    ]
    connection {
      type = "ssh"
      user = "root"
      host = self.network_interface[0].primary_v6_address[0].address
    }
  }
}

resource "ycp_load_balancer_target_group" "mdb-dns_tg" {
  name      = "mdb-dns-tg"
  region_id = var.region_id

  dynamic "target" {
    for_each = ycp_compute_instance.mdb-dns_hosts
    content {
      address   = target.value.network_interface.0.primary_v6_address[0].address
      subnet_id = target.value.network_interface.0.subnet_id
    }
  }
}

resource "ycp_load_balancer_network_load_balancer" "mdb-dns_lb" {
  name      = "mdb-dns-lb"
  region_id = var.region_id
  type      = "EXTERNAL"
  attached_target_group {
    target_group_id = ycp_load_balancer_target_group.mdb-dns_tg.id
    health_check {
      healthy_threshold   = 2
      interval            = "5s"
      name                = "ping"
      timeout             = "2s"
      unhealthy_threshold = 2
      http_options {
        path = "/ping"
        port = 80
      }
    }
  }
  listener_spec {
    port        = 443
    target_port = 443
    name        = "mdb-dns-listener"
    protocol    = "TCP"
    external_address_spec {
      yandex_only = true
      ip_version  = "IPV6"
    }
  }
}

resource "ycp_iam_service_account" "dns" {
  lifecycle {
    prevent_destroy = true
  }
  name               = "dns"
  description        = "SA for setting DNS values for clusters"
  service_account_id = "yc.mdb.dns"
}

resource "ycp_compute_placement_group" "mdb-dns_pg" {
  name = "mdb-dns-pg"
  spread_placement_strategy {
    best_effort            = false
    max_instances_per_node = 1
  }
}
