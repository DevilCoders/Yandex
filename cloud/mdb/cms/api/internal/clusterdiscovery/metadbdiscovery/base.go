package metadbdiscovery

import (
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/clusterdiscovery"
	"a.yandex-team.ru/cloud/mdb/internal/metadb"
)

type MetaDBBasedDiscovery struct {
	mDB metadb.MetaDB
}

func NewMetaDBBasedDiscovery(mDB metadb.MetaDB) clusterdiscovery.OtherLegsDiscovery {
	return &MetaDBBasedDiscovery{
		mDB: mDB,
	}
}
