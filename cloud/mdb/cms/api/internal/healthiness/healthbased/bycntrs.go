package healthbased

import (
	"context"
	"fmt"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/healthiness"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/healthiness/healthbased/healthdbspec"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/models"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func (h *HealthBasedHealthiness) ByInstances(ctx context.Context, instances []models.Instance) (res healthiness.HealthCheckResult, err error) {
	fqdns := make([]string, len(instances))
	for ind, cnt := range instances {
		fqdns[ind] = cnt.FQDN
	}
	neighboursInfo, err := h.health.GetHostNeighboursInfo(ctx, fqdns)
	if err != nil {
		return res, xerrors.Errorf("could not get health response: %w", err)
	}

	for _, instance := range instances {
		info, okNI := neighboursInfo[instance.FQDN]
		if !okNI {
			res.Unknown = append(res.Unknown, healthiness.FQDNCheck{
				Instance: instance,
			})
			continue
		}
		checked := healthiness.FQDNCheck{
			Instance:            instance,
			Cid:                 info.Cid,
			Sid:                 info.Sid,
			Roles:               info.Roles,
			HACluster:           info.HACluster,
			HAShard:             info.HAShard,
			CntTotalInGroup:     info.SameRolesTotal + 1,
			CntAliveLeftInGroup: info.SameRolesAlive,
			StatusUpdatedAt:     info.SameRolesTS,
		}
		if h.rr.OkToLetGo(info) {
			if dateFormatted, isStale := healthdbspec.StaleCondition(info, h.now); isStale {
				checked.HintIs = fmt.Sprintf("stale for %s", dateFormatted)
				res.Stale = append(res.Stale, checked)
			} else {
				res.GiveAway = append(res.GiveAway, checked)
			}
			continue
		}
		if isNotProd(info) {
			res.Ignored = append(res.Ignored, checked)
			checked.HintIs = "is not production cluster"
		} else {
			res.WouldDegrade = append(res.WouldDegrade, checked)
		}
	}
	return res, nil
}
