variable "topic_billing" {
  description = "push client topic"
}

module "push_client_config" {
  source                = "../push_client_config"
  files         = [
      {
          name       = "/var/lib/billing/accounting.log"
          topic      = var.topic_billing
          send_delay = 30
      }
  ]
}

output "rendered" {
  value = module.push_client_config.rendered
}
