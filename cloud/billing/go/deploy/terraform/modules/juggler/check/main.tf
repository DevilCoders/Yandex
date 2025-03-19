// https://docs.yandex-team.ru/juggler/aggregates/basics
variable "namespace" {
  default = "ycloud"
}

variable "host" {}
variable "service" {}

variable "children" {
  default = []
}

variable "description" { default = "" }

variable "pronounce" { default = "" }

variable "tags" { type = list(string) }

variable "refresh_time" {
  type    = number
  default = 180
}

variable "ttl" {
  type    = number
  default = 600
}

variable "check_options" {
  type    = map(string)
  default = {}
}

variable "meta-urls" {
  type = list(object({
    type  = string
    url   = string
    title = string
  }))
  default = []
}

variable "flaps" {
  type = object({
    stable_time   = number
    critical_time = number
  })
  default = {
    critical_time = 0
    stable_time   = 0
  }
}

variable "aggregator" {
  type = object({
    type   = string
    kwargs = any
  })
}

locals {

  check_key = {
    host      = var.host
    namespace = var.namespace
    service   = var.service
  }
  base = merge(
    {
      description = var.description
      tags        = var.tags

      refresh_time  = var.refresh_time
      ttl           = var.ttl
      check_options = var.check_options // empty is ok

      aggregator        = var.aggregator.type
      aggregator_kwargs = var.aggregator.kwargs
    },

    // optional keys
    var.pronounce != "" ? { pronounce = var.pronounce } : {},
    length(var.meta-urls) > 0 ? { meta = { urls = var.meta-urls } } : {},
  )


  ytr_parts = {
    children = [
      for child in var.children : {
        host    = child.host
        service = child.service
        type    = child.group_type
      }
    ]
    flaps    = var.flaps
  }

  legacy_parts = {
    children = var.children
    flaps = {
      stable   = var.flaps.stable_time
      critical = var.flaps.critical_time
    }
  }
  ytr = merge(
    local.base,
    local.ytr_parts,
  )

  built = merge(
    local.check_key,
    local.base,
    local.legacy_parts,
  )
}

output "check" {
  value = local.built
}

output "ytr" {
  value = merge(
    local.check_key,
    {
      raw_yaml = jsonencode(local.ytr)
    }
  )
}
