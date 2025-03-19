package services

import (
	"github.com/spf13/cobra"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/mdb-cli/pkg/common"
)

type serviceType string

const (
	PKG serviceType = "package"
	DB  serviceType = "database"
	API serviceType = "API"
)

type serviceName string

const (
	Worker                serviceName = "worker"
	PythonInternalAPI     serviceName = "py-int-api"
	GoInternalAPI         serviceName = "go-int-api"
	Report                serviceName = "report"
	Health                serviceName = "health"
	MetaDB                serviceName = "metadb"
	WorkerTest            serviceName = "worker-test"
	PythonInternalAPITest serviceName = "py-int-api-test"
	MetaDBTest            serviceName = "metadb-test"
	DNS                   serviceName = "dns"
	Secrets               serviceName = "secrets"
	E2E                   serviceName = "e2e"
	E2ETest               serviceName = "e2e-test"
	Backup                serviceName = "backup"

	CMSAPI      serviceName = "cms-api"
	CMSgRPCAPI  serviceName = "cms-grpcapi"
	CMSAutoduty serviceName = "cms-autoduty"
	CMSDB       serviceName = "cmsdb"
)

type Service struct {
	Group         string
	CloseFilePath optional.String
	PingURL       optional.String
	Type          serviceType
	// SLSs is SLS IDs which we post in release ticket
	SLSs []string
}

type Environment map[serviceName]Service // Name to Service
type Topology map[string]Environment     // Deployment to Environment
var (
	topology = initTopology()
)

func compServices(cmd *cobra.Command, args []string, toComplete string) ([]string, cobra.ShellCompDirective) {
	env := "porto-prod"
	if _, ok := topology[common.FlagValueDeployment]; ok {
		env = common.FlagValueDeployment
	}
	services := make([]string, 0, len(topology[env]))
	for s := range topology[env] {
		services = append(services, string(s))
	}
	return services, cobra.ShellCompDirectiveNoFileComp
}

func initTopology() Topology {
	t := Topology{
		"porto-test": Environment{
			Report: Service{
				Group: "%mdb_report_porto_test",
				Type:  PKG,
			},
			GoInternalAPI: Service{
				Group:         "%mdb_internal_api_porto_test",
				Type:          API,
				CloseFilePath: optional.NewString("/tmp/.mdb-internal-api-close"),
				PingURL:       optional.NewString("http://localhost/ping"),
			},
			CMSAPI: Service{
				Group:   "%mdb_cms_api_porto_test",
				Type:    PKG,
				PingURL: optional.NewString("https://localhost/ping"),
			},
			CMSgRPCAPI: Service{
				Group:         "%mdb_cms_grpcapi_porto_test",
				CloseFilePath: optional.NewString("/tmp/.mdb-cms-close"),
				Type:          API,
				PingURL:       optional.NewString("http://localhost/ping"),
			},
			CMSAutoduty: Service{
				Group: "%mdb_cms_autoduty_porto_test",
				Type:  PKG,
			},
			CMSDB: Service{
				Group: "%mdb_cmsdb_porto_test",
				Type:  DB,
			},
			Health: Service{
				Group:         "%mdb_health_porto_test",
				Type:          API,
				CloseFilePath: optional.NewString("/var/tmp/mdb_health.close"),
				PingURL:       optional.NewString("https://localhost/v1/ping"),
			},
			DNS: Service{
				Group:   "%mdb_dns_porto_test",
				Type:    PKG,
				PingURL: optional.NewString("https://localhost/v1/ping"),
			},
			Secrets: Service{
				Group:         "%mdb_secrets_api_porto_test",
				CloseFilePath: optional.NewString("/tmp/.mdb-secrets-close"),
				Type:          API,
				PingURL:       optional.NewString("https://localhost/v1/ping"),
			},
			Backup: Service{
				Group: "%mdb_backup_porto_test",
				Type:  PKG,
			},
		},
		"porto-prod": Environment{
			Worker: Service{
				Group: "%mdb_worker_porto_prod",
				Type:  PKG,
			},
			WorkerTest: Service{
				Group: "%mdb_worker_porto_test",
				Type:  PKG,
			},
			PythonInternalAPI: Service{
				Group:         "%mdb_api_porto_prod,%mdb_api_admin_porto_prod",
				Type:          API,
				CloseFilePath: optional.NewString("/tmp/.dbaas-internal-api-close"),
				PingURL:       optional.NewString("http://localhost/ping"),
			},
			PythonInternalAPITest: Service{
				Group:         "%mdb_api_porto_test,%mdb_api_admin_porto_test",
				Type:          API,
				CloseFilePath: optional.NewString("/tmp/.dbaas-internal-api-close"),
				PingURL:       optional.NewString("http://localhost/ping"),
			},
			GoInternalAPI: Service{
				Group:         "%mdb_internal_api_porto_prod",
				Type:          API,
				CloseFilePath: optional.NewString("/tmp/.mdb-internal-api-close"),
				PingURL:       optional.NewString("http://localhost/ping"),
			},
			Report: Service{
				Group: "%mdb_report_porto_prod",
				Type:  PKG,
			},
			CMSAPI: Service{
				Group:   "%mdb_cms_api_porto_prod",
				Type:    PKG,
				PingURL: optional.NewString("https://localhost/ping"),
			},
			CMSgRPCAPI: Service{
				Group:         "%mdb_cms_grpcapi_porto_prod",
				CloseFilePath: optional.NewString("/tmp/.mdb-cms-close"),
				Type:          API,
				PingURL:       optional.NewString("http://localhost/ping"),
			},
			CMSAutoduty: Service{
				Group: "%mdb_cms_autoduty_porto_prod",
				Type:  PKG,
			},
			CMSDB: Service{
				Group: "%mdb_cms_db_porto_prod",
				Type:  DB,
			},
			Health: Service{
				Group:         "%mdb_health_porto_prod",
				Type:          API,
				CloseFilePath: optional.NewString("/var/tmp/mdb_health.close"),
				PingURL:       optional.NewString("https://localhost/v1/ping"),
			},
			MetaDB: Service{
				Group: "%mdb_meta_porto_prod",
				Type:  DB,
			},
			MetaDBTest: Service{
				Group: "%mdb_meta_porto_test",
				Type:  DB,
			},
			DNS: Service{
				Group:   "%mdb_dns_porto_prod",
				Type:    PKG,
				PingURL: optional.NewString("https://localhost/v1/ping"),
			},
			Secrets: Service{
				Group:         "%mdb_secrets_api_porto_prod",
				CloseFilePath: optional.NewString("/tmp/.mdb-secrets-close"),
				Type:          API,
				PingURL:       optional.NewString("https://localhost/v1/ping"),
			},
			E2ETest: Service{
				Group: "%mdb_e2e_porto_test",
				Type:  PKG,
			},
			Backup: Service{
				Group: "%mdb_backup_porto_prod",
				Type:  PKG,
			},
		},
		"compute-preprod": Environment{
			Worker: Service{
				Group: "%mdb_worker_compute_preprod",
				Type:  PKG,
			},
			Report: Service{
				Group: "%mdb_report_compute_preprod",
				Type:  PKG,
			},
			CMSgRPCAPI: Service{
				Group:         "%mdb_cms_grpcapi_compute_preprod",
				CloseFilePath: optional.NewString("/tmp/.mdb-cms-close"),
				Type:          API,
				PingURL:       optional.NewString("http://localhost/ping"),
			},
			CMSAutoduty: Service{
				Group: "%mdb_cms_autoduty_compute_preprod",
				Type:  PKG,
			},
			CMSDB: Service{
				Group: "%mdb_cms_db_compute_preprod",
				Type:  DB,
			},
			Health: Service{
				Group:         "%mdb_health_compute_preprod",
				Type:          API,
				CloseFilePath: optional.NewString("/var/tmp/mdb_health.close"),
				PingURL:       optional.NewString("https://localhost/v1/ping"),
			},
			MetaDB: Service{
				Group: "%mdb_meta_compute_preprod",
				Type:  DB,
			},
			DNS: Service{
				Group:   "%mdb_dns_compute_preprod",
				Type:    PKG,
				PingURL: optional.NewString("https://localhost/v1/ping"),
			},
			E2E: Service{
				Group: "%mdb_e2e_compute_preprod",
				Type:  PKG,
			},
			Backup: Service{
				Group: "%mdb_backup_compute_preprod",
				Type:  PKG,
			},
		},
		"compute-prod": Environment{
			Worker: Service{
				Group: "%mdb_worker_compute_prod",
				Type:  PKG,
			},
			PythonInternalAPI: Service{
				Group:         "%mdb_api_compute_prod,%mdb_api_admin_compute_prod",
				Type:          API,
				CloseFilePath: optional.NewString("/tmp/.dbaas-internal-api-close"),
				PingURL:       optional.NewString("http://localhost/ping"),
			},
			GoInternalAPI: Service{
				Group:         "%mdb_internal_api_compute_prod",
				Type:          API,
				CloseFilePath: optional.NewString("/tmp/.mdb-internal-api-close"),
				PingURL:       optional.NewString("http://localhost/ping"),
			},
			Report: Service{
				Group: "%mdb_report_compute_prod",
				Type:  PKG,
			},
			CMSgRPCAPI: Service{
				Group:         "%mdb_cms_grpcapi_compute_prod",
				CloseFilePath: optional.NewString("/tmp/.mdb-cms-close"),
				Type:          API,
				PingURL:       optional.NewString("http://localhost/ping"),
			},
			CMSAutoduty: Service{
				Group: "%mdb_cms_autoduty_compute_prod",
				Type:  PKG,
			},
			CMSDB: Service{
				Group: "%mdb_cms_db_compute_prod",
				Type:  DB,
			},
			Health: Service{
				Group:         "%mdb_health_compute_prod",
				Type:          API,
				CloseFilePath: optional.NewString("/var/tmp/mdb_health.close"),
				PingURL:       optional.NewString("https://localhost/v1/ping"),
			},
			MetaDB: Service{
				Group: "%mdb_meta_compute_prod",
				Type:  DB,
			},
			DNS: Service{
				Group:   "%mdb_dns_compute_prod",
				Type:    PKG,
				PingURL: optional.NewString("https://localhost/v1/ping"),
			},
			Secrets: Service{
				Group:         "%mdb_secrets_api_compute_prod",
				CloseFilePath: optional.NewString("/tmp/.mdb-secrets-close"),
				Type:          API,
				PingURL:       optional.NewString("https://localhost/v1/ping"),
			},
			E2E: Service{
				Group: "%mdb_e2e_compute_prod",
				Type:  PKG,
			},
			Backup: Service{
				Group: "%mdb_backup_compute_prod",
				Type:  PKG,
			},
		},
		"gpn-prod": Environment{
			Worker: Service{
				Group: "%mdb_worker_gpn_prod",
				Type:  PKG,
			},
			PythonInternalAPI: Service{
				Group:         "%mdb_api_gpn_prod",
				Type:          API,
				CloseFilePath: optional.NewString("/tmp/.dbaas-internal-api-close"),
				PingURL:       optional.NewString("http://localhost/ping"),
			},
			GoInternalAPI: Service{
				Group:         "%mdb_internal_api_gpn_prod",
				Type:          API,
				CloseFilePath: optional.NewString("/tmp/.mdb-internal-api-close"),
				PingURL:       optional.NewString("http://localhost/ping"),
			},
			Report: Service{
				Group: "%mdb_report_gpn_prod",
				Type:  PKG,
			},
			Health: Service{
				Group:         "%mdb_health_gpn_prod",
				Type:          API,
				CloseFilePath: optional.NewString("/var/tmp/mdb_health.close"),
				PingURL:       optional.NewString("https://localhost/v1/ping"),
			},
			MetaDB: Service{
				Group: "%mdb_meta_gpn_prod",
				Type:  DB,
			},
			DNS: Service{
				Group:   "%mdb_dns_gpn_prod",
				Type:    PKG,
				PingURL: optional.NewString("https://localhost/v1/ping"),
			},
			Secrets: Service{
				Group:         "%mdb_secrets_api_gpn_prod",
				CloseFilePath: optional.NewString("/tmp/.mdb-secrets-close"),
				Type:          API,
				PingURL:       optional.NewString("https://localhost/v1/ping"),
			},
			E2E: Service{
				Group: "%mdb_e2e_gpn_prod",
				Type:  PKG,
			},
		},
	}

	workerSLS := []string{"dbaas-worker-pkgs", "dbaas-worker-supervised"}
	pyAPISLS := []string{"dbaas-internal-api-pkgs", "internal-api-supervised"}

	service2SLSs := map[serviceName][]string{
		Worker:                workerSLS,
		WorkerTest:            workerSLS,
		PythonInternalAPI:     pyAPISLS,
		PythonInternalAPITest: pyAPISLS,
		GoInternalAPI:         {"mdb-internal-api-pkgs", "mdb-internal-api-service"},
		Report:                {"mdb-maintenance-pkgs", "mdb-katan-pkgs", "mdb-katan-supervised", "mdb-image-releaser-pkgs"},
		CMSAPI:                {"mdb-cms-pkgs", "mdb-cms-service"},
		CMSgRPCAPI:            {"mdb-cms-pkgs", "mdb-cms-instance-api-service"},
		CMSAutoduty:           {"mdb-cms-pkgs", "mdb-cms-instance-autoduty-service"},
		DNS:                   {"mdb-dns-pkgs", "mdb-dns-supervised"},
		Secrets:               {"mdb-secrets-pkgs", "mdb-secrets-service"},
		Health:                {"mdb-health-pkgs", "mdb-health-supervised"},
		Backup:                {"mdb-backup-pkgs", "mdb-backup-worker-service"},
	}

	for env, services := range t {
		for serviceName, s := range services {
			if SLSs, ok := service2SLSs[serviceName]; ok {
				s.SLSs = SLSs
				t[env][serviceName] = s
			}
		}
	}
	return t
}
