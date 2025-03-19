data "sops_file" "grafana" {
  source_file = "${path.root}/grafana_secrets.yaml"
}

provider "grafana" {
  url  = "https://grafana.infra.double.tech"
  auth = data.sops_file.grafana.data["grafana.api_key"]
}

resource "grafana_data_source" "iam_prod" {
  type = "prometheus"
  name = "IAM Prod"
  url  = "https://aps-workspaces.eu-central-1.amazonaws.com/workspaces/ws-3be02953-57fd-494f-8028-c330c57f45e6"

  json_data {
    sigv4_auth            = true
    sigv4_auth_type       = "default"
    sigv4_region          = "eu-central-1"
    sigv4_assume_role_arn = "arn:aws:iam::937784353709:role/prometheus-reader"
  }
}

resource "grafana_data_source" "iam_preprod" {
  type = "prometheus"
  name = "IAM Preprod"
  url  = "https://aps-workspaces.eu-central-1.amazonaws.com/workspaces/ws-de5dc120-03d4-4a4a-9864-43c2c91920ed"

  json_data {
    sigv4_auth            = true
    sigv4_auth_type       = "default"
    sigv4_region          = "eu-central-1"
    sigv4_assume_role_arn = "arn:aws:iam::821159050485:role/prometheus-reader"
  }
}

resource "grafana_dashboard" "iam_duty_deploy" {
  config_json = file("${path.module}/dashboards/iam-duty-deploy.json")
}

resource "grafana_dashboard" "iam_duty" {
  config_json = file("${path.module}/dashboards/iam-duty.json")
}

resource "grafana_dashboard" "iam_duty_grpc_by_host" {
  config_json = file("${path.module}/dashboards/iam-duty_dd-grpc-by-host.json")
}

resource "grafana_dashboard" "iam_duty_grpc_by_method" {
  config_json = file("${path.module}/dashboards/iam-duty_dd-grpc-by-method.json")
}

resource "grafana_dashboard" "iam_duty_grpc_conn_by_host" {
  config_json = file("${path.module}/dashboards/iam-duty_dd-grpc-connections-by-host.json")
}

resource "grafana_dashboard" "iam_duty_jvm_sys_by_host" {
  config_json = file("${path.module}/dashboards/iam-duty_dd-iam-jvm-sys-by-host.json")
}

resource "grafana_dashboard" "iam_duty_errors_by_host" {
  config_json = file("${path.module}/dashboards/iam-duty_dd-iam-service-errors-by-host.json")
}

resource "grafana_dashboard" "iam_duty_tp_by_host" {
  config_json = file("${path.module}/dashboards/iam-duty_dd-iam-tp-by-host.json")
}
