package grpcserver

import (
	api "a.yandex-team.ru/cloud/mdb/cms/api/grpcapi/v1"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/cmsdb"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/dom0discovery"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/dom0discovery/conductorcache"
	"a.yandex-team.ru/cloud/mdb/internal/conductor"
	fqdnlib "a.yandex-team.ru/cloud/mdb/internal/fqdn"
	fqdn "a.yandex-team.ru/cloud/mdb/internal/fqdn/impl"
	"a.yandex-team.ru/cloud/mdb/internal/juggler/push"
	"a.yandex-team.ru/cloud/mdb/internal/metadb"
	"a.yandex-team.ru/library/go/core/log"
)

type InstanceService struct {
	cmsdb  cmsdb.Client
	dom0d  dom0discovery.Dom0Discovery
	mdb    metadb.MetaDB
	cndcl  conductor.Client
	pusher push.Pusher
	log    log.Logger
	auth   *Auth
	cfg    *Config

	conductorCache *conductorcache.Cache
	converter      fqdnlib.Converter
}

var _ api.InstanceServiceServer = &InstanceService{}
var _ api.InstanceOperationServiceServer = &InstanceService{}
var _ api.DutyServiceServer = &InstanceService{}

func NewInstanceService(
	client cmsdb.Client,
	dom0d dom0discovery.Dom0Discovery,
	mdb metadb.MetaDB,
	pusher push.Pusher,
	l log.Logger,
	auth *Auth,
	cndcl conductor.Client,
	conductorCache *conductorcache.Cache,
	cfg *Config,
) *InstanceService {
	return &InstanceService{
		cmsdb:          client,
		dom0d:          dom0d,
		mdb:            mdb,
		pusher:         pusher,
		log:            l,
		auth:           auth,
		cndcl:          cndcl,
		conductorCache: conductorCache,
		cfg:            cfg,
		converter: fqdn.NewConverter(
			cfg.FQDNSuffixes.Controlplane,
			cfg.FQDNSuffixes.UnmanagedDataplane,
			cfg.FQDNSuffixes.ManagedDataplane,
		),
	}
}
