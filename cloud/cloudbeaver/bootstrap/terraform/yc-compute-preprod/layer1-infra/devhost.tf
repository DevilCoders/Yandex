resource "ycp_compute_instance" "devhost" {
  name        = "devhost"
  platform_id = "standard-v2"
  zone_id     = "ru-central1-a"

  resources {
    cores         = 6
    memory        = 12
    core_fraction = 100
  }

  boot_disk {
    disk_spec {
      size     = 30
      image_id = "fdvfcbkab8h6dva8v6ll" // yc --profile preprod compute image get-latest-from-family --folder-id standard-images ubuntu-1604-lts
      type_id  = "network-hdd"
    }
  }

  network_interface {
    subnet_id = "bucj9i1cqha4s8kuh1ma"
    primary_v6_address {}
    primary_v4_address {}
    security_group_ids = [yandex_vpc_security_group.cloudbeaver-devhost-sg.id]
  }

  metadata = {
    ssh-keys           = "ubuntu:ssh-rsa AAAAB3NzaC1yc2EAAAADAQABAAACAQDDIB+ATSgXy3vOvbIsB67zARBEbBj/aL+hCTq8vIZcblMQ7qX6o+ziIXf4IIoAQ5BZihxRFE5pCbxAx/Ed0BcNXDuBK8MwuXw0oT10Cw8xoVREMmVHhMadDy5PZxNFzuz/s7QXBwwUA3FhNMP1VjlYjl10KGYhSQ6bZkg4W7CKnPWV/0XObfb2E6DqUCoU2FHOJXWwWHK/NtERGig9ii22hTQjeO3r0CBGcoYP2l05R+kTvu1Pnbr9BydLJ2mmvFtBE/C7tlBCZ+FAKvQ6JLftC6rAvk4oxkzPVyyfHr51WCqgD2iVwUFEvixhlTXW59cF0k9sWbP0wwUAqYchkCkYUOGR3wm5GE/zYEkAl8fBEnibvrjkFT70FkSh/QTzd4+LSUiDukZB2+e0LWV86OmYGDvqFE0DA3kJ+SHl0G74HzQV/zJZ9WS035avdV2AWcrc3nBbQ8wO16QPZ09IN3WIjAtRLKdtPVk01FfDrzGbegjomfjW+PMBmT0POSQ+o12PE+bwYYXDrwXtGy65N1DRlck8RVv9QKHv5e1IuRUb96uKoCgbj8IzAQcdi+HvltbXUxNnh0vVPaMRlDSFgZGjwaGiaYQzdeOWAjRIi8rQryVOGp9pTSW0oZ9FFDE7rpb913FuycxfwsWNDMLkyF5DvjIIs4u5ntavVif4NN2NYQ== robert@robert-ub14"
    user-data          = "#cloud-config\nbootcmd:\n  - [ cloud-init-per, once, set_google_metadata_tmpl, sh, -c, 'echo \"169.254.169.254 metadata.google.internal\" | tee -a /etc/cloud/templates/hosts.debian.tmpl' ]\n  - [ sh, -c, '/sbin/dhclient -6 -D LL -nw -pf /run/dhclient_ipv6.eth0.pid -lf /var/lib/dhcp/dhclient_ipv6.eth0.leases eth0' ]"
    enable-oslogin     = true
    serial-port-enable = 1
  }
}
