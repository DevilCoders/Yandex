variable "hosts" {
  type    = list(string)
  default = []
}

variable "host_type" { default = "HOST" }

variable "services" {}


output "for_service" {
  value = { for service in var.services :
    service => [for host in var.hosts : {
      host       = host
      service    = service
      group_type = var.host_type
      }
    ]
  }
}
