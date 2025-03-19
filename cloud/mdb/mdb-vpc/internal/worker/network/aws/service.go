package aws

import (
	"net/http"

	"github.com/aws/aws-sdk-go/aws/credentials"

	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/providers/aws"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/vpcdb"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/worker/config"
	"a.yandex-team.ru/library/go/core/log"
)

type Service struct {
	providers aws.Providers
	logger    log.Logger
	db        vpcdb.VPCDB
	cfg       config.AwsControlPlaneConfig
}

func NewService(
	creds *credentials.Credentials,
	region string,
	client *http.Client,
	l log.Logger,
	db vpcdb.VPCDB,
	cfg config.AwsControlPlaneConfig,
) *Service {
	cpSes := aws.NewSession(creds, region, client)
	dpSes := aws.NewSessionAssumeRole(cpSes, cfg.DataplaneRolesArns.ClickHouse, region, client)

	return NewCustomService(
		aws.NewProviders(cpSes, dpSes, region, client),
		l,
		db,
		cfg,
	)
}

func NewCustomService(
	providers aws.Providers,
	l log.Logger,
	db vpcdb.VPCDB,
	cfg config.AwsControlPlaneConfig,
) *Service {
	return &Service{
		providers: providers,
		logger:    l,
		db:        db,
		cfg:       cfg,
	}
}
