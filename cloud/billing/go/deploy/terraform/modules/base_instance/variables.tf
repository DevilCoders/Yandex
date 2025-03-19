variable "installation" {
  default     = "default"
  description = "instance name"
}
variable "name" {
  description = "instance name"
}

variable "hostname_prefix" {
  default     = ""
  description = "Prefix to form instance hostname"
}

variable "hostname_suffix" {
  default     = ""
  description = "Suffix to form instance hostname"
}

variable "host_index" {
  default     = ""
  description = "Index of host in location to form instance hostname"
}

variable "nsdomain" {
  default     = "billing.cloud.yandex.net"
  description = "Domain name of instance"
}

variable "dns_zone_id" {
  default     = ""
  description = "DNS zone id"
}
variable "folder_id" {
  default     = "yc.billing.service-folder"
  description = "Folder for instance"
}

variable "ssh-keys" {
  description = "Instance metdata field 'ssh-keys'"
  default     = ""
}

variable "oslogin" {
  description = "Enable oslogin in instance metadata"
  default     = false
}

variable "skip_update_ssh_keys" {
  default = "false"
}

variable "instance_description" {
  default = "default instance description"
}

variable "osquery_tag" {
  default = "ycloud-svc"
}

variable "instance_platform_id" {
  default     = "standard-v2"
  description = "Instance type. Supported instance types listed at https://cloud.yandex.ru/docs/compute/concepts/vm-platforms)"
}


variable "cores_per_instance" {
  default     = 4
  description = "CPU cores per one instance"
}

variable "core_fraction_per_instance" {
  default     = 100
  description = "CPU core fraction per one instance"
}

variable "disk_per_instance" {
  default     = 128
  description = "Boot disk size per one instance"
}

variable "disk_type" {
  default     = "network-hdd"
  description = "One of disk types. Supported listed at https://nda.ya.ru/3VQZR6"
}

variable "memory_per_instance" {
  default     = 16
  description = "Memory in GB per one instance"
}

variable "image_id" {
  description = "Image ID for instance creation"
}

variable "subnet" {
  description = "instance subnet"
}

variable "skip_underlay" {
  type        = bool
  description = "do not add underlay network"

  default = false
}


///
variable "zone" {
  default = "ru-central1-c"
}

variable "host_suffix_for_zone" {
  type = map(any)

  default = {
    "ru-central1-a" = "vla"
    "ru-central1-b" = "sas"
    "ru-central1-c" = "myt"
  }
}

variable "logging_endpoint_for_installation" {
  type = map(any)

  default = {
    "default"        = ""
    "preprod"        = "api.cloud-preprod.yandex.net:443"
    "preprod-qa"     = "api.cloud-preprod.yandex.net:443"
    "preprod-canary" = "api.cloud-preprod.yandex.net:443"
    "prod"           = "api.cloud.yandex.net:443"
    "prod-qa"        = "api.cloud.yandex.net:443"
    "prod-canary"    = "api.cloud.yandex.net:443"
  }
}

variable "metadata" {
  description = "Additional metadata keys (will be mergged with module-based)"
  type        = map(any)
  default     = {}
}

variable "labels" {
  description = "Additional labels for instance (will be mergbed with module-based)"
  type        = map(any)
  default     = {}
}

variable "service_account_id" {
  default = "yc.billing.svm"
}

variable "logging_group_id" {
  description = "logging group for placing in hosts metadata"
  default     = ""
}

variable "security_group_ids" {
  type    = list(any)
  default = []
}
