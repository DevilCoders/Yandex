resource "ycp_compute_instance" "mdb-worker_hosts" {
  count = 2

  name                      = format(var.name_format, "mdb-worker", count.index + 1, var.zone_suffix)
  zone_id                   = var.zone_id
  platform_id               = "standard-v2"
  fqdn                      = format(var.fqdn_format, "mdb-worker", count.index + 1, var.zone_suffix, var.fqdn_suffix)
  allow_stopping_for_update = true

  resources {
    core_fraction = 50
    cores         = 2
    memory        = 4
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

resource "ycp_iam_service_account" "worker" {
  lifecycle {
    prevent_destroy = true
  }
  name               = "worker"
  service_account_id = "yc.mdb.worker"
}

resource "ycp_compute_placement_group" "mdb-worker_pg" {
  name = "mdb-worker-pg"
  spread_placement_strategy {
    best_effort            = false
    max_instances_per_node = 1
  }
}
