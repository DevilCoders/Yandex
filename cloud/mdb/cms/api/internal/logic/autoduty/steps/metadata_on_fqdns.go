package steps

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/dom0discovery"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/helpers/shipments"
	"a.yandex-team.ru/cloud/mdb/cms/api/pkg/opmetas"
	"a.yandex-team.ru/cloud/mdb/deploy/api/pkg/deployapi"
)

type MetadataOnFQDNSStep struct {
	deploy deployapi.Client
	dom0   dom0discovery.Dom0Discovery
	cfg    shipments.AwaitShipmentConfig
}

func (s *MetadataOnFQDNSStep) GetStepName() string {
	return "metadata on fqdns"
}

func (s *MetadataOnFQDNSStep) RunStep(ctx context.Context, execCtx *InstructionCtx) RunResult {
	rd := execCtx.GetActualRD()
	containers, err := s.dom0.Dom0Instances(ctx, rd.R.MustOneFQDN())
	if err != nil {
		return errorListingDBM(err)
	}
	deployWrapper := bringMetadataWrapper(s.deploy, s.cfg)
	opMeta := rd.D.OpsLog.BringMetadataOnFQNDs
	if opMeta != nil {
		return WaitShipment(ctx, opMeta, deployWrapper, rd)
	} else {
		fqdns := make([]string, len(containers.WellKnown))
		for ind, container := range containers.WellKnown {
			fqdns[ind] = container.FQDN
		}
		opMeta = opmetas.NewBringMetadataMeta()
		rd.D.OpsLog.BringMetadataOnFQNDs = opMeta
		return CreateShipment(ctx, fqdns, opMeta, deployWrapper)
	}
}

func NewMetadataOnFQDNSStepStep(deploy deployapi.Client, cfg shipments.AwaitShipmentConfig, dom0 dom0discovery.Dom0Discovery) DecisionStep {
	return &MetadataOnFQDNSStep{
		deploy: deploy,
		cfg:    cfg,
		dom0:   dom0,
	}
}
