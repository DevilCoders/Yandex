provider "yandex" {
  endpoint  = "api.cloud-preprod.yandex.net:443"
  cloud_id  = "aoe47jkb996097rp5rnr" // yc-tools
  folder_id = "aoeerlb2k499hurvqm0v" // infra
  zone      = "ru-central1-a"
}

terraform {
  required_providers {
    yandex = {
      version = ">= 0.55.0"
      source  = "yandex-cloud/yandex"
    }
  }
  required_version = ">= 0.14"
}

resource "yandex_compute_instance" "oslogin_test_1" {
  name        = "oslogin-test-1"
  platform_id = "standard-v2"

  resources {
    cores         = 2
    memory        = 4
    core_fraction = 5
  }

  boot_disk {
    initialize_params {
      image_id = "fdvfcbkab8h6dva8v6ll" // yc --profile preprod compute image get-latest-from-family --folder-id standard-images ubuntu-1604-lts
    }
  }

  network_interface {
    subnet_id = "bucfs8b7ub3nbpu27s0s" // cloud-tools-lab-nets-ru-central1-a
    ipv6      = "true"
    nat       = false
  }

  metadata = {
    // temporary authentication using ssh certificates for oslogin deployment
    ssh-keys       = "ubuntu:cert-authority,principals=\"nautim\" ecdsa-sha2-nistp521 AAAAE2VjZHNhLXNoYTItbmlzdHA1MjEAAAAIbmlzdHA1MjEAAACFBAFxDHbZHZCx72sztvWsFSJVUhW+fngANgR5iiIWy9gicOdjYOy0pxpb1U8PqWmJYJYOWLwtfm/f9qnSV3Q/wAX/igCAwdYuwsWtx/0eBp2G3ECIUJzEQRme+TrtECpJHyLFCJonE/Cg24JlQ8N6mVjrQZwLiVjCdVwRFGoXxb3F1N4SHg== bastion"
    // add metadata.google.internal to hosts file template
    user-data      = "#cloud-config\nbootcmd:\n  - [ cloud-init-per, once, set_google_metadata_tmpl, sh, -c, 'echo \"169.254.169.254 metadata.google.internal\" | tee -a /etc/cloud/templates/hosts.debian.tmpl' ]\n  - [ sh, -c, '/sbin/dhclient -6 -D LL -nw -pf /run/dhclient_ipv6.eth0.pid -lf /var/lib/dhcp/dhclient_ipv6.eth0.leases eth0' ]"
    enable-oslogin = true
  }

  allow_stopping_for_update = true

  connection {
    type         = "ssh"
    timeout      = "5m"
    host         = yandex_compute_instance.oslogin_test_1.network_interface.0.ipv6_address
    user         = "ubuntu"
    agent        = true
    bastion_host = "lb.bastion.cloud.yandex.net"
    bastion_port = 22
    bastion_user = "nautim"
  }

  provisioner "remote-exec" {
    inline = [
      "cat /home/ubuntu/.ssh/authorized_keys",
      "sudo mkdir -p /srv/oslogin/salt",
      "sudo mkdir -p /srv/oslogin/pillar",
      "sudo chown -R ubuntu:ubuntu /srv/oslogin",
    ]
  }

  provisioner "file" {
    source      = "salt"
    destination = "/srv/oslogin"
  }

  provisioner "file" {
    source      = "pillar"
    destination = "/srv/oslogin"
  }

  provisioner "remote-exec" {
    inline = [
      "sudo apt-get update",
      "sleep 10",
      "sudo apt-get install -y salt-minion",
      "sudo salt-call --local --file-root /srv/oslogin/salt --pillar-root /srv/oslogin/pillar state.highstate",
      "sudo apt-get -y purge salt-minion salt-common",
      "sudo rm -rf /var/log/salt /var/cache/salt",
      "rm /home/ubuntu/.ssh/authorized_keys",
    ]
  }
}
