package grpcserver

import (
	"context"
	"time"

	"github.com/opentracing/opentracing-go"

	"a.yandex-team.ru/cloud/mdb/internal/conductor"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func (s *InstanceService) RefreshConductorCache(ctx context.Context) {
	ticker := time.NewTicker(s.cfg.ConductorCacheRefreshInterval)
	defer ticker.Stop()

	for {
		select {
		case <-ticker.C:
			_ = s.DoRefreshConductorCache(ctx)

		case <-ctx.Done():
			return
		}
	}
}

func (s *InstanceService) DoRefreshConductorCache(ctx context.Context) error {
	sp, ctx := opentracing.StartSpanFromContext(ctx, "DoRefreshConductorCache")
	defer sp.Finish()
	ctxlog.Debug(ctx, s.log, "Start refresh conductor cache")
	hosts, err := s.conductorHosts(ctx)
	if err != nil {
		return err
	}
	s.conductorCache.UpdateHosts(hosts)
	ctxlog.Debug(ctx, s.log, "Finish refresh conductor cache")
	return nil
}

func (s *InstanceService) conductorHosts(ctx context.Context) (map[string][]string, error) {
	data, err := s.cndcl.ExecuterData(ctx, s.cfg.ConductorProject)
	if err != nil {
		return nil, xerrors.Errorf("conductor executer_data: %w", err)
	}
	res := make(map[string][]string)
	groups := make(map[string][]string)

	for fqdn, host := range data.Hosts {
		res[fqdn] = conductorGroupStack(host.Group, data.Groups, groups)
	}

	return res, nil
}

func conductorGroupStack(group string, groups map[string]conductor.GroupExecuterData, groupsCache map[string][]string) []string {
	if res, ok := groupsCache[group]; ok {
		return res
	}
	res := []string{group}
	for _, parent := range groups[group].Parents {
		res = append(res, conductorGroupStack(parent, groups, groupsCache)...)
	}

	groupsCache[group] = res
	return res
}
