data "external" "lockbox-secret-getter" {
  program = [
      "${path.module}/command.sh",
      var.yc_profile,
      var.id,
      var.value_key
  ]
}
