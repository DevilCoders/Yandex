locals {

  all_files = concat(["/var/lib/docker/containers/*/*.log"], var.additional_files)


  config = <<-EOT
    %{for file in local.all_files}
    ${file} {
        size 200M
        rotate 5
        nocompress
        copytruncate
        missingok
    }
    %{endfor}
    EOT

  configs = {
    "/etc/logrotate.d/logrotate-app.conf" = local.config
  }
}