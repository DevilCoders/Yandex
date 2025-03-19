package jugglerbased

import (
	"context"
	"fmt"
	"time"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/healthiness"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/models"
	"a.yandex-team.ru/cloud/mdb/internal/conductor"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

const NotDbaas = "host is not auto-managed"

func (h *JugglerBasedHealthiness) isAnyWellKnownGroupAParent(ctx context.Context, gInfo conductor.GroupInfo, instance string) (string, models.KnownGroups, error) {
	pgInfo, err := h.conductor.ParentGroup(ctx, gInfo)
	if err != nil {
		return "", models.KnownGroups{}, err
	}
	if WKGroup, ok := h.knownGroupMap[pgInfo.Name]; ok {
		return pgInfo.Name, WKGroup, nil
	}
	return h.isAnyWellKnownGroupAParent(ctx, pgInfo, instance)
}

func (h *JugglerBasedHealthiness) findWellKnownGroup(ctx context.Context, instances []models.Instance) (map[string]models.KnownGroups, error) {
	r := map[string]models.KnownGroups{}
	fqdns := make([]string, len(instances))
	for ind, inst := range instances {
		fqdns[ind] = inst.FQDN
	}
	hostsInfo, err := h.conductor.HostsInfo(ctx, fqdns)
	if err != nil {
		return nil, xerrors.Errorf("could not get hosts info from conductor: %w", err)
	}
	for _, info := range hostsInfo {
		ctx := ctxlog.WithFields(ctx, log.String("group", info.Group), log.String("instance", info.FQDN))
		if groups, ok := h.knownGroupMap[info.Group]; ok {
			r[info.FQDN] = groups
			ctxlog.Infof(ctx, h.L, "Found group %s for instance %s in config", info.Group, info.FQDN)
			continue
		}
		gInfo, err := h.conductor.GroupInfoByName(ctx, info.Group)
		if err != nil {
			return nil, xerrors.Errorf("could not get group info from conductor: %w", err)
		}
		parentName, wkGroup, err := h.isAnyWellKnownGroupAParent(ctx, gInfo, info.FQDN)
		if err != nil {
			if semerr.IsNotFound(err) {
				ctxlog.Debugf(ctx, h.L, "Did not find any group for instance %s in config", info.FQDN)
				continue
			}
			return nil, xerrors.Errorf("could not get parents from conductor: %w", err)
		}
		r[info.FQDN] = wkGroup
		ctxlog.Infof(ctx, h.L, "Found group %s for instance %s in config", parentName, info.FQDN)
	}
	return r, nil
}

func (h *JugglerBasedHealthiness) ByInstances(ctx context.Context, instances []models.Instance) (healthiness.HealthCheckResult, error) {
	r := healthiness.HealthCheckResult{}
	groupsToCheck, err := h.findWellKnownGroup(ctx, instances)
	if err != nil {
		return r, err
	}
	for _, iToCheck := range instances {
		ctx := ctxlog.WithFields(ctx, log.String("i", iToCheck.FQDN))
		gToCheck, found := groupsToCheck[iToCheck.FQDN]
		if !found {
			r.Unknown = append(r.Unknown, healthiness.FQDNCheck{
				Instance: iToCheck,
				Cid:      NotDbaas,
				Sid:      NotDbaas,
				HintIs:   "no suitable conductor group in config exists, choose one and add it to config",
			})
			continue
		}
		if gToCheck.MaxUnhealthy == 0 {
			r.Unknown = append(r.Unknown, healthiness.FQDNCheck{
				Instance: iToCheck,
				Cid:      "",
				Sid:      "",
				HintIs:   "marked unknown, cause MaxUnhealthy is zero",
			})
			continue
		}
		groupHosts, err := h.conductor.GroupToHosts(ctx, iToCheck.ConductorGroup, conductor.GroupToHostsAttrs{})
		if err != nil {
			return r, xerrors.Errorf("could not resolve group %s to hosts in conductor: %w", iToCheck.ConductorGroup, err)
		}
		check, err := h.jugglerChecker.Check(ctx, groupHosts, gToCheck.Checks, time.Now())
		if err != nil {
			return r, err
		}
		allowedToDegradeCnt := gToCheck.MaxUnhealthy - len(check.NotOK)
		if allowedToDegradeCnt > 0 {
			r.GiveAway = append(r.GiveAway, healthiness.FQDNCheck{
				Instance: iToCheck,
				Cid:      NotDbaas,
				Sid:      NotDbaas,
				Roles:    []string{gToCheck.Type},
			})
		} else {
			r.WouldDegrade = append(r.WouldDegrade, healthiness.FQDNCheck{
				Instance: iToCheck,
				Cid:      NotDbaas,
				Sid:      NotDbaas,
				Roles:    []string{gToCheck.Type},
				HintIs:   fmt.Sprintf("%d already degraded", len(check.NotOK)),
			})
		}
	}
	return r, nil
}
