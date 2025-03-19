package steps

import (
	"context"
	"fmt"
	"strings"
	"time"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/dom0discovery"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/helpers/juggler"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/helpers/shipments"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/models"
	"a.yandex-team.ru/cloud/mdb/cms/api/pkg/opmetas"
	"a.yandex-team.ru/cloud/mdb/deploy/api/pkg/deployapi"
	deployModels "a.yandex-team.ru/cloud/mdb/deploy/api/pkg/models"
	"a.yandex-team.ru/cloud/mdb/internal/dbm"
	"a.yandex-team.ru/cloud/mdb/internal/encodingutil"
	jugglerapi "a.yandex-team.ru/cloud/mdb/internal/juggler"
	"a.yandex-team.ru/library/go/slices"
)

type MetadataOnClusterNodesStep struct {
	dom0           dom0discovery.Dom0Discovery
	deploy         deployapi.Client
	juggler        jugglerapi.API
	dbm            dbm.Client
	cfg            shipments.AwaitShipmentConfig
	shipmentConfig shipments.AwaitShipmentConfig
	jgrlConf       juggler.JugglerConfig
}

func (s *MetadataOnClusterNodesStep) GetStepName() string {
	return "metadata on other nodes"
}

func (s *MetadataOnClusterNodesStep) GroupClusterNodesByUnreachableCheck(ctx context.Context, containers []models.Instance) (juggler.FQDNGroupByJugglerCheck, error) {
	CIDs := make([]string, len(containers))
	requestedFQDNs := make([]string, len(containers))
	for ind, cntr := range containers {
		CIDs[ind] = cntr.DBMClusterName
		requestedFQDNs[ind] = cntr.FQDN
	}
	var checkReachable []string
	for _, CID := range CIDs {
		nodes, err := s.dbm.ClusterContainers(ctx, CID)
		if err != nil {
			return juggler.FQDNGroupByJugglerCheck{}, err
		}
		for _, cntr := range nodes {
			if slices.ContainsString(requestedFQDNs, cntr.FQDN) {
				continue
			}
			checkReachable = append(checkReachable, cntr.FQDN)
		}
	}
	checker := juggler.NewJugglerReachabilityChecker(s.juggler, s.jgrlConf.UnreachableServiceWindowMin)
	return checker.Check(ctx, checkReachable, []string{juggler.Unreachable}, time.Now())
}

func (s *MetadataOnClusterNodesStep) RunStep(ctx context.Context, execCtx *InstructionCtx) RunResult {
	rd := execCtx.GetActualRD()
	containers, err := s.dom0.Dom0Instances(ctx, rd.R.MustOneFQDN())
	if err != nil {
		return errorListingDBM(err)
	}
	deployWrapper := bringMetadataWrapper(s.deploy, s.cfg)
	opMeta := rd.D.OpsLog.BringMetadataOnClusters
	if opMeta != nil && len(opMeta.GetShipments()) > 0 {
		return WaitShipment(ctx, opMeta, deployWrapper, rd)
	} else {
		availability, err := s.GroupClusterNodesByUnreachableCheck(ctx, containers.WellKnown)
		if err != nil {
			return waitWithErrAndMessage(err, "could not get reachable nodes on clusters")
		}
		if len(availability.NotOK) > 0 {
			return waitWithMessage(fmt.Sprintf("Make these containers reachable and CMS will continue (they are on OTHER dom0s): %s", strings.Join(availability.NotOK, ", ")))
		}
		opMeta = opmetas.NewBringMetadataMeta()
		rd.D.OpsLog.BringMetadataOnClusters = opMeta
		return CreateShipment(ctx, availability.OK, opMeta, deployWrapper)
	}
}

func bringMetadataWrapper(deploy deployapi.Client, cfg shipments.AwaitShipmentConfig) shipments.AwaitShipment {
	var cmds = []deployModels.CommandDef{{
		Type:    "state.sls",
		Args:    []string{"components.dbaas-operations.metadata"},
		Timeout: encodingutil.FromDuration(time.Minute * time.Duration(cfg.Timeout)),
	}}
	return shipments.NewDeployWrapperFromCfg(
		cmds,
		deploy,
		cfg,
	)
}

func NewMetadataOnClusterNodesStep(
	dom0 dom0discovery.Dom0Discovery,
	deploy deployapi.Client,
	cfg shipments.AwaitShipmentConfig,
	jgrlConf juggler.JugglerConfig,
	juggler jugglerapi.API,
	dbm dbm.Client,
) DecisionStep {
	return &MetadataOnClusterNodesStep{
		dom0:           dom0,
		deploy:         deploy,
		shipmentConfig: cfg,
		jgrlConf:       jgrlConf,
		dbm:            dbm,
		juggler:        juggler,
	}
}
