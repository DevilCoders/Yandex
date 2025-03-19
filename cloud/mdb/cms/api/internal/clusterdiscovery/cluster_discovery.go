package clusterdiscovery

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/internal/metadb"
)

//go:generate ../../../../scripts/mockgen.sh OtherLegsDiscovery

type Neighbourhood struct {
	Others []metadb.Host
	Self   metadb.Host
	ID     string
}

type OtherLegsDiscovery interface {
	FindInShardOrSubcidByFQDN(ctx context.Context, fqdn string) (Neighbourhood, error)
}
