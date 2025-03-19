
module "constants" {
  source      = "../constants"
  environment = "preprod"
}

provider "yandex" {
  endpoint  = module.constants.by_environment.yc_endpoint
  cloud_id  = "b1gfcpod5hbd1ivs7dav"
  folder_id = var.folder_id
  zone      = "ru-central1-a"
}

module "user_data" {
  source       = "../../modules/user_data"
  yandex_token = var.yandex_token

  extra_user_data = {
    disk_setup = {
      "/dev/disk/by-id/virtio-models" = {
        table_type = "mbr"
        overwrite = false
      }
    }
    fs_setup = [{
      filesystem = "ext4"
      device = "/dev/disk/by-id/virtio-models"
      overwrite = false
    }]
    mounts = [["/dev/disk/by-id/virtio-models", "/mnt/models", "auto", "defaults", "0", "0"]]
  }
}

resource "yandex_compute_disk" "models" {
  name = "models"
  type = "network-ssd"
  size = "500"
}

resource "yandex_compute_instance" "stt-model-downloader" {
  name               = "stt-model-downloader"
  platform_id        = "standard-v2"
  service_account_id = "ajetcebmqrguvq01bfmn"

  resources {
    cores  = 2
    memory = 8
  }

  boot_disk {
    initialize_params {
      image_id = "fd8pilnduh8qjs35mjq0"
    }
  }

  network_interface {
    subnet_id = module.constants.by_environment.subnets.ru-central1-a
    nat       = true
    ipv4      = false
    ipv6      = true
  }

  secondary_disk {
    device_name = "models"
    disk_id = yandex_compute_disk.models.id
    mode = "READ_WRITE"
  }

  metadata = {
    user-data = module.user_data.rendered
  }
}
