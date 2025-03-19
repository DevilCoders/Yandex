package steps

import (
	"context"
	"fmt"
	"strings"
	"time"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/clusterdiscovery/metadbdiscovery"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/dom0discovery"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/helpers/shipments"
	"a.yandex-team.ru/cloud/mdb/cms/api/pkg/opmetas"
	"a.yandex-team.ru/cloud/mdb/deploy/api/pkg/deployapi"
	deployModels "a.yandex-team.ru/cloud/mdb/deploy/api/pkg/models"
	"a.yandex-team.ru/cloud/mdb/internal/encodingutil"
	"a.yandex-team.ru/cloud/mdb/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type WhipMasterFrom struct {
	TargetFQDN string
	FromHosts  []string
}

type EnsureNoPrimariesStep struct {
	deploy                      deployapi.Client
	dom0d                       dom0discovery.Dom0Discovery
	metadb                      metadb.MetaDB
	sameRegionClusterLegsMethod func(ctx context.Context, mDB metadb.MetaDB, fqdn string) (WhipMasterFrom, error)
}

func (s *EnsureNoPrimariesStep) GetStepName() string {
	return "whip primaries away"
}

var (
	isNotHaError = xerrors.New("is not ha")
)

func SameRegionClusterLegsByFQDN(ctx context.Context, mDB metadb.MetaDB, fqdn string) (WhipMasterFrom, error) {
	result := WhipMasterFrom{
		TargetFQDN: fqdn,
		FromHosts:  []string{},
	}
	dscvr := metadbdiscovery.NewMetaDBBasedDiscovery(mDB)
	neighbourhood, err := dscvr.FindInShardOrSubcidByFQDN(ctx, fqdn)
	if err != nil {
		return WhipMasterFrom{}, err
	}
	if len(neighbourhood.Others) == 0 {
		return WhipMasterFrom{}, isNotHaError
	}
	result.FromHosts = append(result.FromHosts, neighbourhood.Self.FQDN)
	for _, h := range neighbourhood.Others {
		if neighbourhood.Self.Geo == h.Geo {
			result.FromHosts = append(result.FromHosts, h.FQDN)
		}
	}
	return result, nil
}

func PrefixFromFQDNs(fqdns []string) []string {
	result := make([]string, len(fqdns))
	for i, fqdn := range fqdns {
		result[i] = strings.Split(fqdn, ".")[0]
	}
	return result
}

func (s *EnsureNoPrimariesStep) RunStep(ctx context.Context, execCtx *InstructionCtx) RunResult {
	rd := execCtx.GetActualRD()
	containers, err := s.dom0d.Dom0Instances(ctx, rd.R.MustOneFQDN())
	if err != nil {
		return errorListingDBM(err)
	}
	if len(containers.WellKnown) == 0 {
		return continueWithMessage("no containers")
	}
	allKnownContainerFQDNs := make([]string, len(containers.WellKnown))
	for index, container := range containers.WellKnown {
		allKnownContainerFQDNs[index] = container.FQDN
	}
	stepState := rd.D.OpsLog.EnsureNoPrimary
	createResult := shipments.WrapperCreateResult{}
	var whipFQDNs []WhipMasterFrom
	if stepState != nil && len(stepState.GetShipments()) > 0 {
		deployWrapper := ensureNoPrimaryDeployWrapper(s.deploy, []string{})
		resultFromPreviousRun := WaitShipment(ctx, stepState, deployWrapper, rd)
		if resultFromPreviousRun.Action != AfterStepContinue {
			// state from previous run did not finish successfully
			return resultFromPreviousRun
		}
		if resultFromPreviousRun.Error != nil {
			return resultFromPreviousRun
		}
		// check we created shipments for all containers from the previous time
		stateShipments := stepState.GetShipments()
		containersWithShipmentsFinished := make([]string, 0, len(stateShipments))
		for fqdn := range stepState.GetShipments() {
			// TODO: test me
			containersWithShipmentsFinished = append(containersWithShipmentsFinished, fqdn)
		}
		difference := SlicesDifference(allKnownContainerFQDNs, containersWithShipmentsFinished)
		if len(difference) == 0 {
			return resultFromPreviousRun // all DONE!
		}
		for _, targetFQDN := range difference {
			whipFromLegs, err := s.sameRegionClusterLegsMethod(ctx, s.metadb, targetFQDN)
			if err != nil {
				if semerr.IsNotFound(err) {
					createResult.AddNotHA(targetFQDN)
					continue // a Control Plane host, S3 host
				}
				if xerrors.Is(err, isNotHaError) {
					createResult.AddNotHA(targetFQDN)
					continue // will not try to whip master from one legged clusters
				}
				resultFromPreviousRun.Error = err
				return resultFromPreviousRun
			}
			whipFQDNs = append(whipFQDNs, whipFromLegs)
		}
		if len(whipFQDNs) == 0 {
			return resultFromPreviousRun
		}
	} else {
		stepState = opmetas.NewEnsureNoPrimaryMeta()
		for _, targetFQDN := range allKnownContainerFQDNs {
			whipFromLegs, err := s.sameRegionClusterLegsMethod(ctx, s.metadb, targetFQDN)
			if err != nil {
				if semerr.IsNotFound(err) {
					createResult.AddNotHA(targetFQDN)
					continue // a Control Plane host, S3 host
				}
				if xerrors.Is(err, isNotHaError) {
					createResult.AddNotHA(targetFQDN)
					continue // will not try to whip master from one legged clusters
				}
				return waitWithErrAndMessage(err, "error while getting other legs")
			}
			whipFQDNs = append(whipFQDNs, whipFromLegs)
		}
		if len(whipFQDNs) == 0 {
			return continueWithMessage(fmt.Sprintf("nothing to be done, %d containers, but none to whip", len(containers.WellKnown)))
		}
	}
	rd.D.OpsLog.EnsureNoPrimary = stepState
	for _, whipTarget := range whipFQDNs {
		deployWrapper := ensureNoPrimaryDeployWrapper(s.deploy, PrefixFromFQDNs(whipTarget.FromHosts))
		tmpResult := deployWrapper.CreateShipment(ctx, []string{whipTarget.TargetFQDN}, stepState)
		createResult.NotCreated = append(createResult.NotCreated, tmpResult.NotCreated...)
		createResult.NotInDeploy = append(createResult.NotInDeploy, tmpResult.NotInDeploy...)
		createResult.Success = append(createResult.Success, tmpResult.Success...)
		createResult.Error = tmpResult.Error
		if createResult.Error != nil {
			break
		}

	}
	return MessageForUserOnCreate(createResult)
}

func NewCustomEnsureNoPrimariesStep(
	deploy deployapi.Client,
	dom0d dom0discovery.Dom0Discovery,
	mdb metadb.MetaDB,
	sameRegionClusterLegs func(ctx context.Context, mDB metadb.MetaDB, fqdn string) (WhipMasterFrom, error)) DecisionStep {
	return &EnsureNoPrimariesStep{
		deploy:                      deploy,
		dom0d:                       dom0d,
		metadb:                      mdb,
		sameRegionClusterLegsMethod: sameRegionClusterLegs,
	}
}

func NewEnsureNoPrimariesStep(deploy deployapi.Client, dom0d dom0discovery.Dom0Discovery, mdb metadb.MetaDB) DecisionStep {
	return NewCustomEnsureNoPrimariesStep(deploy, dom0d, mdb, SameRegionClusterLegsByFQDN)
}

func ensureNoPrimaryDeployWrapper(d deployapi.Client, fromHostPrefixes []string) shipments.AwaitShipment {
	const Timeout = 20
	var cmds = []deployModels.CommandDef{{
		Type: "cmd.run",
		Args: []string{shipments.ShExecIfPresent(
			"/usr/local/yandex/ensure_no_primary.sh",
			fmt.Sprintf("--from-hosts=%s", strings.Join(fromHostPrefixes, ",")),
		)},
		Timeout: encodingutil.FromDuration(time.Minute * time.Duration(Timeout)),
	}}
	return shipments.NewDeployWrapper(
		cmds,
		d,
		1,
		1,
		Timeout+1,
	)
}
