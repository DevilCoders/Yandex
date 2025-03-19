output "lb_arn" {
  value = aws_lb.nlb.arn
}

output "helm_lb_config" {
  value = {
    serviceType  = var.service_type
    targetGroups = [ for k, v in module.target : {
      alias       = k
      serviceName = var.targets[k].service_name
      targetType  = var.target_type
      port        = v.target_group_port
      portName    = var.targets[k].port_name
      arn         = v.target_group_arn
    }]
  }
}

output "lb_dns_name" {
  value = aws_lb.nlb.dns_name
}

output "lb_zone_id" {
  value = aws_lb.nlb.zone_id
}
