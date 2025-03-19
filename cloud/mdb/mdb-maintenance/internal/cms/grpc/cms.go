package grpc

import (
	"a.yandex-team.ru/cloud/mdb/cms/api/pkg/instanceclient"
	mntcms "a.yandex-team.ru/cloud/mdb/mdb-maintenance/internal/cms"
	"a.yandex-team.ru/cloud/mdb/mdb-maintenance/internal/metadb"
)

type GRPCCMS struct {
	cms  instanceclient.InstanceClient
	meta metadb.MetaDB
}

var _ mntcms.CMS = &GRPCCMS{}

func NewGRPCCMS(
	instanceClient instanceclient.InstanceClient,
	meta metadb.MetaDB,
) mntcms.CMS {
	return &GRPCCMS{
		cms:  instanceClient,
		meta: meta,
	}
}
