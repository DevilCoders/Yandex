variable "ycp_profile" {
  type        = string
  description = "Name of the ycp profile"
}

variable "yc_endpoint" {
  type        = string
  description = "Endpoint for skm"
}

variable "log_level" {
  type        = string
  description = "Accounting log level"
  default     = "info"
}


variable "folder_id" {
  type        = string
  description = "The ID of folder to deploy accounting service"
  // Well-known id of "accounting" folder in "yc.vpc.accounting" cloud. See https://bb.yandex-team.ru/projects/CLOUD/repos/bootstrap-templates/browse/terraform/*/vpc.tf
  default = "yc.vpc.accounting.svms"
}

variable "environment" {
  type        = string
  description = "Name of cloud environment, i.e. prod or testing"
}

variable "accounting_network_hbf_enabled" {
  type        = bool
  description = "Should HBF be enabled on accounting network. Should be false only for private clouds."
  default     = true
}

variable "accounting_network_subnet_zones" {
  type        = list(string)
  description = "List of zones for accounting network subnets."
  default     = ["ru-central1-a", "ru-central1-b", "ru-central1-c"]
}

variable "accounting_network_ipv6_cidrs" {
  type        = map(string)
  description = "IPv6 CIDRs of accounting network for each availability zones. Doesn't have default values, because there are differ set of prefixes on each zone. I.e. see http://netbox.cloud.yandex.net/ipam/prefixes/1269/ for TESTING VLA. Should be /64 prefixes."
}

variable "accounting_network_ipv4_cidrs" {
  type        = map(string)
  description = "IPv4 CIDRs of accounting network for each availability zones."
  default = {
    ru-central1-a = "172.16.0.0/16"
    ru-central1-b = "172.17.0.0/16"
    ru-central1-c = "172.18.0.0/16"
  }
}

variable "dns_zone" {
  type        = string
  description = "DNS zone for all service VMs. Should be equal to YTR settings. I.e. vpc-accounting.svc.cloud.yandex.net"
}

variable "dns_zone_id" {
  type        = string
  description = "Id for passed dns zone."
}

variable "hc_network_ipv6" {
  type        = string
  description = "IPv6 healthcheck network"
}

variable "logbroker_endpoint" {
  type        = string
  description = "Container logbroker endpoint"
}

variable "logbroker_accounting_database" {
  type        = string
  description = "VPC accounting logbroker database"
}

variable "logbroker_billing_database" {
  type        = string
  description = "Billing logbroker database"
}

variable "solomon_url" {
  type        = string
  description = "Cloud logbroker installation"
}

variable "logbroker_accounting_topic" {
  type        = string
  description = "Accounting metric topic"
  default     = "/yc.vpc.virtual-network/vpc-accounting/accounting"
}

variable "logbroker_billing_topic" {
  type        = string
  description = "Accounting billing topic"
  default     = "/yc.billing.service-cloud/billing-sdn-traffic-nfc"
}

variable "logbroker_loadbalancer_topic" {
  type        = string
  description = "Accounting loadbalancer topic"
  default     = "/yc.billing.service-cloud/billing-lb-traffic-nfc"
}

variable "logbroker_billing_consumer" {
  type        = string
  description = "Accounting billing consumer"
  default     = "/yc.vpc.virtual-network/vpc-accounting/billing-consumer"
}

variable "logbroker_billing_test_topic" {
  type        = string
  description = "Accounting billing test topic"
  default     = "/yc.vpc.virtual-network/vpc-accounting/billing-test"
}

variable "logbroker_billing_test_consumer" {
  type        = string
  description = "Accounting billing test consumer"
  default     = "/yc.vpc.virtual-network/vpc-accounting/billing-test-consumer"
}

variable "logbroker_loadbalancer_test_topic" {
  type        = string
  description = "Accounting loadbalancer topic"
  default     = "/yc.vpc.virtual-network/vpc-accounting/loadbalancer-test"
}

variable "logbroker_accounting_test_topic" {
  type        = string
  description = "Accounting testing topic"
  default     = "/yc.vpc.virtual-network/vpc-accounting/accounting-test"
}

variable "logbroker_test_consumer" {
  type        = string
  description = "Accounting billing consumer"
  default     = "/yc.vpc.virtual-network/vpc-accounting/test-consumer"
}

variable "accounting_max_read_partitions" {
  type        = number
  default     = 9
  description = "Max partitions readed by one instance"
}

variable "accounting_instances_count" {
  type        = number
  default     = 3
  description = "Max accounting instances count"
}

variable "test_instances_count" {
  type        = number
  default     = 3
  description = "Max test instances count"
}

variable "solomon_pull_interval" {
  type        = string
  default     = "60s"
  description = "Solomon pull interval"
}

variable "unchanged_metrics_lifetime" {
  type        = string
  default     = "40"
  description = "Solomon metrics cache size"
}

variable "accounting_image" {
  type        = string
  description = "Id of compute image for accounting instances. Images may be found in folder or in artifacts of hopper job https://teamcity.aw.cloud.yandex.net/project.html?projectId=VPC_VirtualNetwork_Images&tab=projectOverview"
}

variable "vm_platform_id" {
  type        = string
  description = "Platform of VMs"
  default     = "standard-v2"
}
