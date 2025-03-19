package grpcserver

import (
	"context"

	api "a.yandex-team.ru/cloud/mdb/cms/api/grpcapi/v1"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/dom0discovery"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/ctxlog"
)

func resolveForOneDom0(dom0d dom0discovery.Dom0Discovery, ctx context.Context, dom0 string, l log.Logger) ([]*api.Instance, error) {

	containers, err := dom0d.Dom0Instances(ctx, dom0)
	if err != nil {
		return nil, err
	}

	if len(containers.Unknown) > 0 {
		ctxlog.Debugf(ctx, l, "Unknown containers: %v", containers.Unknown)
	}

	allKnownContainers := make([]*api.Instance, len(containers.WellKnown))
	for i, container := range containers.WellKnown {
		allKnownContainers[i] = &api.Instance{Dom0: dom0, Fqdn: container.FQDN}
	}
	return allKnownContainers, err
}

func (s *InstanceService) ResolveInstancesByDom0(ctx context.Context, req *api.ResolveInstancesByDom0Request) (*api.InstanceListResponce, error) {

	if s.cfg.IsCompute {
		return nil, semerr.NotImplemented("ResolveInstancesByDom0 not implemented for compute installation")
	}

	var allKnownContainers []*api.Instance

	for _, dom0 := range req.Dom0 {
		oneDom0Containers, err := resolveForOneDom0(s.dom0d, ctx, dom0, s.log)
		if err != nil {
			return nil, err
		}
		allKnownContainers = append(allKnownContainers, oneDom0Containers...)
	}

	resp := &api.InstanceListResponce{
		Instance: allKnownContainers,
	}

	return resp, nil
}

func (s *InstanceService) Dom0Instances(ctx context.Context, request *api.Dom0InstancesRequest) (*api.Dom0InstancesResponse, error) {
	if s.cfg.IsCompute {
		return nil, semerr.NotImplemented("Dom0Instances not implemented for compute installation")
	}

	containers, err := s.dom0d.Dom0Instances(ctx, request.Dom0)
	if err != nil {
		return nil, err
	}

	result := &api.Dom0InstancesResponse{
		WellKnown: make([]*api.WellKnownInstance, len(containers.WellKnown)),
		Unknown:   containers.Unknown,
	}
	for i, instance := range containers.WellKnown {
		result.WellKnown[i] = &api.WellKnownInstance{
			ConductorGroup: instance.ConductorGroup,
			DbmClusterName: instance.DBMClusterName,
			Fqdn:           instance.FQDN,
		}
	}
	return result, nil
}
