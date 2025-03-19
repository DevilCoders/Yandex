output "solomon-agent" {
    value = data.template_file.solomon-agent.rendered
}

output "solomon-agent-extra" {
    value = data.template_file.solomon-agent-extra.rendered
}

output "sa_public_key" {
    value = jsondecode(module.yav_secret.secret)["public_key"]
}

output "sa_private_key" {
    value = jsondecode(module.yav_secret.secret)["private_key"]
}
