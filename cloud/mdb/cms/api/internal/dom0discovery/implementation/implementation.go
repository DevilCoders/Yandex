package implementation

import (
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/dom0discovery"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/dom0discovery/conductorcache"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/models"
	"a.yandex-team.ru/cloud/mdb/internal/dbm"
	"a.yandex-team.ru/library/go/core/log"
)

type Dom0DiscoveryImpl struct {
	dbm     dbm.Client
	whiteLG map[string]bool
	blackLG map[string]bool
	L       log.Logger
	cache   *conductorcache.Cache
}

func NewDom0DiscoveryImpl(
	wGroups []models.KnownGroups,
	bGroups []string,
	dbm dbm.Client,
	l log.Logger,
	conductorCache *conductorcache.Cache,
) dom0discovery.Dom0Discovery {
	whiteGL := map[string]bool{}
	for _, cg := range wGroups {
		for _, name := range cg.CGroups {
			whiteGL[name] = true
		}
	}
	blackGL := map[string]bool{}
	for _, name := range bGroups {
		blackGL[name] = true
	}
	return &Dom0DiscoveryImpl{
		whiteLG: whiteGL,
		blackLG: blackGL,
		dbm:     dbm,
		L:       l,
		cache:   conductorCache,
	}
}
