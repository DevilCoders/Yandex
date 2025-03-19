variable "yc_folder" {
  description = "Yandex Cloud Folder ID where resources will be created"
  default = "yc.search.backends"
}

variable "yc_zone" {
  description = "Yandex Cloud default Zone for provisioned resources"
  default     = "il1"
}

variable "yc-search-backend-pg" {
  default = "alkarsa78oo528c9s0md"
}

variable "yc-search-indexer-pg" {
  default = "alkbj4a8hb1e38hdkvh3"
}

variable "yc-search-proxy-pg" {
  default = "alkis5pok3ph15aibtdl"
}

variable "yc-search-queue-pg" {
  default = "alkjd0c4n38vg40tdme2"
}

variable "host_group" {
  description = "SVM host group"
  default     = "service"
}
variable "service_account_id" {
  default = "yc.search.sa"
}

variable "deploy_service_account_id" {
  default = "yc.search.sa"
}

variable "proxy_image_id" {
  default = "alk78c2mtfe460rhpthu" # ycp --profile israel compute image list --folder-id yc.search.backends
}

variable "backend_image_id" {
  default = "alk78c2mtfe460rhpthu" # ycp --profile israel compute image list --folder-id yc.search.backends
}

variable "marketplace_backend_image_id" {
  default = "alk78c2mtfe460rhpthu" # ycp --profile israel compute image list --folder-id yc.search.backends
}

variable "indexer_image_id" {
  default = "alk78c2mtfe460rhpthu" # ycp --profile israel compute image list --folder-id yc.search.backends
}

variable "queue_image_id" {
  default = "alk78c2mtfe460rhpthu" # ycp --profile israel compute image list --folder-id yc.search.backends
}

variable "yc_zones" {
  description = "Yandex Cloud Zones to deploy in"
  default = ["il1-a"]
}

variable "ipv6_addresses" {
  default = ["", "", ""]
}

variable "ipv4_addresses" {
  default = ["", "", ""]
}

variable "subnets" {
  type = "map"

  default = {
    "il1-a" = "ddkp07unovshhodvqs32"
  }
}

variable "yc_zone_suffix" {
  default = {
    "il1-a" = "il1-a"
  }
}

variable "hostname_suffix" {
  default = "search.yandexcloud.co.il"
}

variable "instance_disk_size" {
  description = "Boot disk size in GB per one instance"
  default     = "12"
}

variable "instance_disk_type" {
  description = "Boot disk type, list of types: https://cloud.yandex.ru/docs/compute/concepts/disk#disks_types"
  default     = "network-hdd"
}

variable "instance_platform_id" {
  description = "instance platform id"
  default     = "standard-v2"
}

variable "instance_pci_topology_id" {
  default = "V2"
}

variable "security_group_ids" {
  default = []
}

variable "proxy_prefix" {
    default = "proxy"
    description = "display prefix name of instance"
}

variable "backend_prefix" {
    default = "backend"
    description = "display prefix name of instance"
}

variable "marketplace_backend_prefix" {
    default = "mrkt-backend"
    description = "display prefix name of instance"
}

variable "indexer_prefix" {
    default = "indexer"
    description = "display prefix name of instance"
}

variable "queue_prefix" {
    default = "queue"
    description = "display prefix name of instance"
}

variable "max_unavailable" {
  default = 1
}

variable "startup_duration" {
  default = "30s"
}

variable "dns_zone_id" {
  default = "b442k1k4po1j4585m08l"
}

variable "healthcheck_port" {
  default     = 8080
  description = "HTTP port for healthchecking"
}

variable "healthcheck_queue_port" {
  default     = 8081
  description = "HTTP port for healthchecking"
}

variable "healthcheck_path" {
  default     = "/ping"
  description = "HTTP path for healthchecking"
}

variable "ig_healthcheck_timeout" {
  default     = "5s"
  description = "Timeout for server to reply to IG healthcheck"
}

variable "ig_healthcheck_interval" {
  default     = "6s"
  description = "The interval between IG healthchecks"
}

variable "ig_healthcheck_healthy" {
  default     = 2
  description = "The number of OK responses to consider IG healthcheck OK"
}

variable "ig_healthcheck_unhealthy" {
  default     = 5
  description = "The number of OK responses to consider IG healthcheck unhealthy"
}

variable "max_checking_health_duration" {
  default     = "600s"
  description = "Amount of time to wait for successful healthcheck after instance start"
}
