variable "ycp_profile" {
  type = string
  description = "Name of the ycp profile"
}

variable "yc_endpoint" {
  type = string
  description = "Endpoint for skm"
}

variable "log_level" {
  type = string
  description = "Accounting log level"
  default = "info"
}

variable "folder_id" {
  type = string
  description = "The ID of folder to deploy load_test service"
  // Well-known id of "load_test" folder in "yc.vpc.load.test" cloud.
  default = "yc.vpc.load.test.folder"
}

variable "environment" {
  type = string
  description = "Name of cloud environment, i.e. prod or testing"
}

variable "vpc_overlay_net" {
  type = number
  description = "ProjectId for load_test network. I.e. 64557 (0xfc2d) for https://racktables.yandex-team.ru/index.php?page=services&tab=projects&project_name=_CLOUD_MR_PROBER_TESTING_NETS_."
} 

variable "load_test_network_hbf_enabled" {
  type = bool
  description = "Should HBF be enabled on load_test network. Should be false only for private clouds."
  default = true
}

variable "load_test_network_subnet_zones" {
  type = list(string)
  description = "List of zones for load_test network subnets."
  default = ["ru-central1-a", "ru-central1-b", "ru-central1-c"]
}

variable "vpc_load_test_network_ipv6_cidrs" {
  type = map(string)
  description = "IPv6 CIDRs of load_test network for each availability zones. Doesn't have default values, because there are differ set of prefixes on each zone. I.e. see http://netbox.cloud.yandex.net/ipam/prefixes/1269/ for TESTING VLA. Should be /64 prefixes."
}

variable "load_test_network_ipv4_cidrs" {
  type = map(string)
  description = "IPv4 CIDRs of load_test network for each availability zones."
  default = {
    ru-central1-a = "172.16.0.0/16"
    ru-central1-b = "172.17.0.0/16"
    ru-central1-c = "172.18.0.0/16"
  }
}

variable "dns_zone" {
  type = string
  description = "DNS zone for all service VMs. Should be equal to YTR settings. I.e. vpc-accounting.svc.cloud.yandex.net"
}

variable "dns_zone_id" {
  type = string
  description = "Id for passed dns zone."
}


variable "jump_image" {
  type = string
  description = "Id of compute image for load_test instances. Images may be found in folder or in artifacts of hopper job https://teamcity.aw.cloud.yandex.net/project.html?projectId=VPC_VirtualNetwork_Images&tab=projectOverview"
}
