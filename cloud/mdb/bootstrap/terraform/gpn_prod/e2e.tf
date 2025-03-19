resource "ycp_compute_instance" "mdb-e2e_hosts" {
  count                     = 1
  name                      = format(var.name_format, "mdb-e2e", count.index + 1, var.zone_suffix)
  zone_id                   = var.zone_id
  platform_id               = "standard-v2"
  fqdn                      = format(var.fqdn_format, "mdb-e2e", count.index + 1, var.zone_suffix, var.fqdn_suffix)
  allow_stopping_for_update = true

  resources {
    core_fraction = 20
    cores         = 2
    memory        = 2
  }
  boot_disk {
    disk_spec {
      image_id = "d8olrajbtfbhkri3v4q4"
      size     = 30
    }
  }

  network_interface {
    subnet_id = data.ycp_vpc_subnet.mdb-e2e-nets-ru-gpn-spb99.subnet_id
    primary_v4_address {}
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
      host = self.network_interface[1].primary_v6_address[0].address
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
      host = self.network_interface[1].primary_v6_address[0].address
    }
  }
}

resource "ycp_iam_service_account" "mdb-e2e-dataproc_sa" {
  lifecycle {
    prevent_destroy = true
  }
  name               = "mdb-e2e-dataproc"
  description        = "Performs e2e tests"
  service_account_id = "yc.mdb.e2e-dataproc"
}

resource "ycp_storage_bucket" "mdb-e2e-dataproc_buckets" {
  bucket = "mdb-e2e-dataproc"
}
