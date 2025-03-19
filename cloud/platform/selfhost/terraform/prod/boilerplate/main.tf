provider "yandex" {
  token     = "${var.yc_token}"
  folder_id = "${var.yc_folder}"
  zone = "ru-central1-c"
}

module "boilerplate-instance-group" {
  source          = "../../modules/kubelet_instance_group_ytr"
  name_prefix     = "boilerplate-prod"
  role_name       = "boilerplate"
  conductor_group = "boilerplate"

  instance_group_size = "2"
  ipv4_addrs          = ["", ""]
  ipv6_addrs          = ["", ""]
  subnets             = "${var.subnets}"

  cores_per_instance         = "${var.instance_cores}"
  core_fraction_per_instance = 5
  memory_per_instance        = "${var.instance_memory}"
  disk_per_instance          = "${var.instance_disk_size}"
  image_id                   = "${var.image_id}"

  configs       = ""
  infra-configs = ""

  podmanifest   = ""
  docker-config = ""
  ssh-keys      = ""

  skip_update_ssh_keys = false

  metadata = {}

  labels = {}
}

// Instance params

variable "image_id" {
  default = "fd8r1ftsr03kjalu71jq"
}

variable "instance_cores" {
  description = "Cores per one instance"
  default     = "1"
}

variable "instance_memory" {
  description = "Memory in GB per one instance"
  default     = "2"
}

variable "instance_disk_size" {
  description = "Boot disk size in GB per one instance"
  default     = "12"
}

// Instance group placement params

variable "subnets" {
  type = "map"

  default = {
    "ru-central1-a" = "e9b9e47n23i7a9a6iha7"
    "ru-central1-b" = "e2lt4ehf8hf49v67ubot"
    "ru-central1-c" = "b0c7crr1buiddqjmuhn7"
  }
}

variable "yc_folder" {
  description = "Yandex Cloud Folder ID where resources will be created"
  default     = "b1gbqp3l3t8hjt70assh"                                   // xgen
}

variable "yc_token" {
  description = "Yandex Cloud security OAuth token"
}
