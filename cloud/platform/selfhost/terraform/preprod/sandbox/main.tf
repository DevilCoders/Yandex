provider "yandex" {
  endpoint  = ""
  token     = "${var.yc_token}"
  folder_id = "${var.yc_folder}"
  zone      = "${var.yc_zone}"
}

resource "yandex_compute_instance" "foobar" {
  name        = "instance-with-ssh-and-nat"
  description = "VM created by terraform-provider-yandex"

  resources {
    cores  = 1
    memory = 2
  }

  boot_disk {
    initialize_params {
      image_id = "fd8dvtfpeskabitc3qlk" # it's ubuntu-1804-lts
    }
  }

  network_interface {
    subnet_id = "${yandex_vpc_subnet.foo.id}"
    nat       = true
  }

  metadata = {
    foo      = "bar"
    ssh-keys = "ubuntu:${file("${var.public_key_path}")}"
  }

  labels = {
    my_key       = "my_value"
    my_other_key = "my_other_value"
  }

  provisioner "remote-exec" {
    connection {
      user = "ubuntu"
    }

    inline = [
      "echo 'This instance was provisioned by Terraform.' | sudo tee /etc/motd",
    ]
  }
}

resource "yandex_vpc_network" "foo" {}

resource "yandex_vpc_subnet" "foo" {
  name = "it-my-subnet"

  // TODO: remove 'zone' when attr for resource will became Optional:true
  zone           = "${var.yc_zone}"
  network_id     = "${yandex_vpc_network.foo.id}"
  v4_cidr_blocks = ["192.168.192.0/24"]
}
