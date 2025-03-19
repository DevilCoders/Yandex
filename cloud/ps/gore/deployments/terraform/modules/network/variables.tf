variable "project_id" {
  type = number
  description = "ProjectId for GoRe default network. I.e. 64563 (0xfc33) for https://racktables.yandex-team.ru/index.php?page=services&tab=projects&project_name=_CLOUD_GORE_PREPROD_NETS_."
}

variable "hbf_enabled" {
  type = bool
  description = "Should HBF be enabled on GoRe default network. Should be false only for private clouds."
  default = true
}

variable "zones" {
  type = list(string)
  description = "List of zones for GoRe default network subnets."
  default = ["ru-central1-a", "ru-central1-b", "ru-central1-c"]
}

variable "ipv6_cidrs" {
  type = map(string)
  description = "IPv6 CIDRs of GoRe default network for each availability zones. Doesn't have default values, because there are differ set of prefixes on each zone. I.e. see http://netbox.cloud.yandex.net/ipam/prefixes/1269/ for TESTING VLA. Should be /64 prefixes."
}

variable "ipv4_cidrs" {
  type = map(string)
  description = "IPv4 CIDRs of GoRe default network for each availability zones."
  default = {
    ru-central1-a = "172.16.0.0/16"
    ru-central1-b = "172.17.0.0/16"
    ru-central1-c = "172.18.0.0/16"
  }
}

