variable "skip_unreach_service" {
  type    = string
  default = ""
}

locals {
  unreach_args = {
    value = {
      unreach_mode    = "skip"
      unreach_service = [{ check = ":${var.skip_unreach_service}" }]
    }
  }

  lkp = var.skip_unreach_service == "" ? "" : "value"
}

output "immediate_crit" {
  value = {
    type   = "logic_or"
    kwargs = lookup(local.unreach_args, local.lkp, {})
  }
}

output "crit_if_any" {
  value = {
    type = "logic_or"
    kwargs = merge(
      lookup(local.unreach_args, local.lkp, {}),
      { nodata_mode = "force_ok" },
    )
  }
}
