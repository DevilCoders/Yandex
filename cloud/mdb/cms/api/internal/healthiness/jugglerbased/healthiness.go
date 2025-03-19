package jugglerbased

import (
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/healthiness"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/helpers/juggler"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/models"
	"a.yandex-team.ru/cloud/mdb/internal/conductor"
	"a.yandex-team.ru/library/go/core/log"
)

type JugglerBasedHealthiness struct {
	conductor      conductor.Client
	knownGroupMap  map[string]models.KnownGroups
	jugglerChecker juggler.JugglerChecker
	L              log.Logger
}

func NewJugglerBasedHealthiness(cncl conductor.Client, groups []models.KnownGroups, jchk juggler.JugglerChecker, log log.Logger) healthiness.Healthiness {
	kgm := map[string]models.KnownGroups{}
	for _, gs := range groups {
		for _, g := range gs.CGroups {
			kgm[g] = gs
		}
	}
	return &JugglerBasedHealthiness{
		conductor:      cncl,
		knownGroupMap:  kgm,
		jugglerChecker: jchk,
		L:              log,
	}
}
