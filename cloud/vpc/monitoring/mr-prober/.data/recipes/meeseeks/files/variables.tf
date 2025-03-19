variable "prefix" {
  type = string
  description = "Prefix for all objects VMs created in this cluster"
}

variable "zones" {
  type = list(string)
  description = "List of zones for subnets."
  default = ["ru-central1-a", "ru-central1-b", "ru-central1-c"]
}

variable "monitoring_network_ipv4_cidrs" {
  type = map(string)
  description = "IPv4 CIDRs of network for each availability zone"
  default = {
    ru-central1-a = "10.1.0.0/16"
    ru-central1-b = "10.2.0.0/16"
    ru-central1-c = "10.3.0.0/16"
    il1-a = "10.1.0.0/16"
  }
}

variable "monitoring_network_ipv6_cidrs" {
  type = map(string)
  description = "IPv6 CIDRs of network for each availability zone"
}

variable "monitoring_network_hbf_enabled" {
  type = bool
  description = "Should HBF be enabled on Mr. Prober's monitoring network. Should be false only for private clouds."
  default = true
}

variable "label_environment" {
  type = string
  description = "Environment used for label env in instances"
}

variable "compute_nodes" {
  type = list(string)
  description = "List of compute nodes for instances creating"
}

variable "use_service_slot" {
    type = bool
    default = false
}

variable "mr_prober_agent_image_name" {
  type = string
  description = "The name of Mr. Prober agent compute image in https://storage.yandexcloud.net/yc-vpc-packer-export/mr-prober/agent/"
  default = "fd81qruuv96qaek976qq"
}

variable "mr_prober_agent_docker_image_version" {
  type        = string
  description = "The version of Mr. Prober Agent docker image in https://console.cloud.yandex.ru/folders/yc.vpc.mr-prober/container-registry/registries/crpni6s1s1aujltb5vv7/overview"
}

variable "use_conductor" {
  type = bool
  description = "Should conductor group and hosts be created"
  default = false
}

variable "target_count_per_zone" {
  type = number
  description = "Count of internal targets for ICMP and HTTP checks per AZ"
  default = 2

  validation {
    condition = var.target_count_per_zone <= 5
    error_message = "Target count per zone should be not more than 5."
  }
}

variable "target_platform_id" {
  type = string
  description = "Platform ID for target VMs"
  default = "standard-v2"
}
