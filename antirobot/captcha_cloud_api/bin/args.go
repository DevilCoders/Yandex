// ya make . -r && ./captcha_cloud_api --debug --ydb-endpoint ydb-ru-prestable.yandex.net:2135 --ydb-database /ru-prestable/captcha/test/captcha --ydb-token $(cat ~/.ydb_token)

package main

import (
	"flag"
	"log"
	"os"
)

type Args struct {
	Debug                        bool
	TestSleepDelay               int
	Profile                      string
	SkipAuthorization            bool
	Addr                         string
	YdbEndpoint                  string
	YdbDatabase                  string
	YdbToken                     string
	YandexInternalRootCACertPath string
	IAMAccessServiceEndpoint     string
	ResourceManagerEndpoint      string
	ResourceManagerInsecure      bool
	AccessServiceInsecure        bool
	DefaultCaptchasQuota         int64
	UnifiedAgentURI              string
	CaptchaIDPrefix              string
	AllowedSitesMaxSize          int64
	//nolint:ST1003
	StyleJsonMaxSize int64
}

func ParseArgs(inputArgs []string) (args Args) {
	flagSet := flag.NewFlagSet("captcha_cloud_api", flag.PanicOnError)

	flagSet.BoolVar(&args.Debug, "debug", false, "enable debug output")
	flagSet.IntVar(&args.TestSleepDelay, "test-sleep-delay", 0, "")

	flagSet.StringVar(&args.Profile, "profile", "", "prod|preprod")
	flagSet.BoolVar(&args.SkipAuthorization, "skip-authorization", false, "skip authorization")
	flagSet.StringVar(&args.Addr, "addr", ":15000", "address to listen")
	flagSet.StringVar(&args.YdbEndpoint, "ydb-endpoint", "", "YDB endpoint")
	flagSet.StringVar(&args.YdbDatabase, "ydb-database", "", "YDB database")
	flagSet.StringVar(&args.YdbToken, "ydb-token", os.Getenv("YDB_TOKEN"), "YDB token")
	flagSet.StringVar(&args.YandexInternalRootCACertPath, "yandex-internal-root-ca-cert-path", "/etc/ssl/certs/YandexInternalRootCA.pem", "Internal SSL cert path")
	flagSet.StringVar(&args.IAMAccessServiceEndpoint, "iam-access-service-endpoint", "", "IAM Access service endpoint")
	flagSet.StringVar(&args.ResourceManagerEndpoint, "resource-manager-endpoint", "", "Resource manager endpoint")
	flagSet.BoolVar(&args.ResourceManagerInsecure, "resource-manager-insecure", false, "do not use TLS to resource manager")
	flagSet.BoolVar(&args.AccessServiceInsecure, "access-service-insecure", false, "do not use TLS to access service")
	flagSet.Int64Var(&args.DefaultCaptchasQuota, "default-captchas-quota", 10, "maximum number of captchas per cloud")
	flagSet.StringVar(&args.UnifiedAgentURI, "unified-agent-uri", "", "unified agent GRPC URI")
	flagSet.StringVar(&args.CaptchaIDPrefix, "captcha-id-prefix", "", "prefix for generated captcha ID")
	flagSet.Int64Var(&args.AllowedSitesMaxSize, "allowed-sites-max-size", 7000, "maximum total size of captcha.AllowedSites in bytes")
	flagSet.Int64Var(&args.StyleJsonMaxSize, "style-json-max-size", 7000, "maximum size of captcha.StyleJson in bytes")

	// Shouldn't return an error since we use flag.PanicOnError.
	_ = flagSet.Parse(inputArgs)

	switch args.Profile {
	case "prod":
		if args.IAMAccessServiceEndpoint == "" {
			args.IAMAccessServiceEndpoint = "as.private-api.cloud.yandex.net:4286"
		}
		if args.ResourceManagerEndpoint == "" {
			args.ResourceManagerEndpoint = "rm.private-api.cloud.yandex.net:4284"
		}
		if args.CaptchaIDPrefix == "" {
			args.CaptchaIDPrefix = "bpn" // see https://bb.yandex-team.ru/projects/CLOUD/repos/cloud-go/browse/private-api/endpoint_ids/ids-pre-prod.yaml
		}
		if args.YdbEndpoint == "" {
			args.YdbEndpoint = "ydb-eu.yandex.net:2135"
		}
		if args.YdbDatabase == "" {
			args.YdbDatabase = "/eu/captcha/prod/captcha"
		}
	case "preprod":
		if args.IAMAccessServiceEndpoint == "" {
			args.IAMAccessServiceEndpoint = "as.private-api.cloud-preprod.yandex.net:4286"
		}
		if args.ResourceManagerEndpoint == "" {
			args.ResourceManagerEndpoint = "rm.private-api.cloud-preprod.yandex.net:4284"
		}
		if args.CaptchaIDPrefix == "" {
			args.CaptchaIDPrefix = "eo0" // see https://bb.yandex-team.ru/projects/CLOUD/repos/cloud-go/browse/private-api/endpoint_ids/ids-prod.yaml
		}
		if args.YdbEndpoint == "" {
			args.YdbEndpoint = "ydb-ru-prestable.yandex.net:2135"
		}
		if args.YdbDatabase == "" {
			args.YdbDatabase = "/ru-prestable/captcha/test/captcha"
		}
	case "":
		// nothing to do
	default:
		log.Fatalf("Invalid argument '--profile'. Use 'prod' or 'preprod' or nothing")
	}

	if !args.SkipAuthorization && args.IAMAccessServiceEndpoint == "" {
		log.Fatalf("Missing required argument '--iam-access-service-endpoint' or '--profile'")
	}
	if args.ResourceManagerEndpoint == "" {
		log.Fatalf("Missing required argument '--resource-manager-endpoint' or '--profile'")
	}
	if args.CaptchaIDPrefix == "" {
		args.CaptchaIDPrefix = "xxx"
	}
	if args.YdbEndpoint == "" {
		log.Fatalf("Missing required argument '--ydb-endpoint' or '--profile'")
	}
	if args.YdbDatabase == "" {
		log.Fatalf("Missing required argument '--ydb-database' or '--profile'")
	}
	if args.YdbToken == "" {
		log.Println("Missing required argument '--ydb-token' or set YDB_TOKEN environment variable")
	}

	return
}
