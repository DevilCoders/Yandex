package config

import (
	"a.yandex-team.ru/cloud/mdb/internal/pretty"
	"a.yandex-team.ru/library/go/core/log"
)

func DefaultConfig() Config {
	return Config{
		LogLevel:          log.InfoLevel,
		Output:            pretty.YAMLFormat,
		CAPath:            FormatAllCAFilePath(DefaultConfigPath),
		DefaultDeployment: DeploymentNamePortoTest,
		Deployments: map[string]DeploymentConfig{
			DeploymentNameComputeProd:    defaultDeploymentConfigComputeProd(),
			DeploymentNameComputePreprod: defaultDeploymentConfigComputePreprod(),
			DeploymentNamePortoProd:      defaultDeploymentConfigPortoProd(),
			DeploymentNamePortoTest:      defaultDeploymentConfigPortoTest(),
			DeploymentNameGpnProd:        defaultDeploymentConfigGpnProd(),
		},
	}
}

func defaultDeploymentConfigComputeProd() DeploymentConfig {
	return DeploymentConfig{
		DeployAPIURL:                 "https://mdb-deploy-api.private-api.yandexcloud.net",
		MDBUIURI:                     "https://ui-compute-prod.db.yandex-team.ru/",
		MDBAPIURI:                    "https://mdb.private-api.cloud.yandex.net",
		IAMURI:                       "https://identity.private-api.cloud.yandex.net:14336",
		TokenServiceHost:             "ts.private-api.cloud.yandex.net:4282",
		CMSHost:                      "mdb-cmsgrpcapi.private-api.cloud.yandex.net:443",
		ControlplaneFQDNSuffix:       "yandexcloud.net",
		UnamangedDataplaneFQDNSuffix: "mdb.yandexcloud.net",
		ManagedDataplaneFQDNSuffix:   "db.yandex.net",
		Federation: DeploymentFederation{
			ID:       "yc.yandex-team.federation",
			Endpoint: "https://console.cloud.yandex.ru/federations/",
		},
	}
}

func defaultDeploymentConfigComputePreprod() DeploymentConfig {
	return DeploymentConfig{
		DeployAPIURL:                 "https://mdb-deploy-api.private-api.cloud-preprod.yandex.net",
		MDBUIURI:                     "https://ui-preprod.db.yandex-team.ru/",
		MDBAPIURI:                    "https://mdb.private-api.cloud-preprod.yandex.net",
		IAMURI:                       "https://identity.private-api.cloud-preprod.yandex.net:14336",
		TokenServiceHost:             "ts.private-api.cloud-preprod.yandex.net:4282",
		CMSHost:                      "mdb-cmsgrpcapi.private-api.cloud-preprod.yandex.net:443",
		ControlplaneFQDNSuffix:       "cloud-preprod.yandex.net",
		UnamangedDataplaneFQDNSuffix: "mdb.cloud-preprod.yandex.net",
		ManagedDataplaneFQDNSuffix:   "db.yandex.net",
		Federation: DeploymentFederation{
			ID:       "yc.yandex-team.federation",
			Endpoint: "https://console-preprod.cloud.yandex.ru/federations/",
		},
		KubeCtx:       "yc-compute-preprod-cp",
		KubeNamespace: "preprod",
	}
}

func defaultDeploymentConfigPortoProd() DeploymentConfig {
	return DeploymentConfig{
		DeployAPIURL:     "https://deploy-api.db.yandex-team.ru",
		MDBAPIURI:        "https://internal-api.db.yandex-team.ru",
		MDBUIURI:         "https://ui-prod.db.yandex-team.ru/",
		IAMURI:           "https://iam.cloud.yandex-team.ru",
		TokenServiceHost: "ts.cloud.yandex-team.ru:4282",
		CMSHost:          "mdb-cmsgrpcapi.db.yandex.net:443",
	}
}

func defaultDeploymentConfigPortoTest() DeploymentConfig {
	return DeploymentConfig{
		DeployAPIURL:     "https://deploy-api-test.db.yandex-team.ru",
		MDBAPIURI:        "https://internal-api-test.db.yandex-team.ru",
		MDBUIURI:         "https://ui.db.yandex-team.ru/",
		IAMURI:           "https://iam.cloud.yandex-team.ru",
		TokenServiceHost: "ts.cloud.yandex-team.ru:4282",
		CMSHost:          "mdb-cmsgrpcapi-test.db.yandex.net:443",
	}
}

func defaultDeploymentConfigGpnProd() DeploymentConfig {
	return DeploymentConfig{
		DeployAPIURL:                 "https://mdb-deploy.private-api.gpn.yandexcloud.net",
		MDBAPIURI:                    "https://mdb-internal.private-api.gpn.yandexcloud.ne",
		IAMURI:                       "https://identity.private-api.ycp.gpn.yandexcloud.net",
		TokenServiceHost:             "ts.private-api.gpn.yandexcloud.net:14282",
		ControlplaneFQDNSuffix:       "gpn.yandexcloud.net",
		UnamangedDataplaneFQDNSuffix: "mdb.gpn.yandexcloud.net",
		ManagedDataplaneFQDNSuffix:   "int.gpn.yandexcloud.net",
		Federation: DeploymentFederation{
			ID:       "yc.yandex-team.federation",
			Endpoint: "https://console.gpn.yandexcloud.net/federations/",
		},
		CAPath: FormatGPNCAFilePath(DefaultConfigPath),
	}
}
