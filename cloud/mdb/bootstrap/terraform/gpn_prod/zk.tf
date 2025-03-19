resource "ycp_compute_instance" "mdb-zk_hosts" {
  lifecycle {
    prevent_destroy = true
  }
  count = 3

  name                      = format(var.name_format, "mdb-zk", count.index + 1, var.zone_suffix)
  zone_id                   = var.zone_id
  platform_id               = "standard-v2"
  fqdn                      = format(var.fqdn_format, "mdb-zk", count.index + 1, var.zone_suffix, var.fqdn_suffix)
  allow_stopping_for_update = true

  resources {
    core_fraction = 50
    cores         = 2
    memory        = 8
  }

  boot_disk {
    disk_spec {
      image_id = "d8ocvca3ul3o1n0lqs13"
      size     = 20
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

resource "ycp_compute_placement_group" "mdb-zk_pg" {
  name = "mdb-zk-pg"
  spread_placement_strategy {
    best_effort            = false
    max_instances_per_node = 1
  }
}
