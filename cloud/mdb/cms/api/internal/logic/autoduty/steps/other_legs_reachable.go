package steps

import (
	"context"
	"fmt"
	"time"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/clusterdiscovery/metadbdiscovery"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/dom0discovery"
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/helpers/juggler"
	jugglerapi "a.yandex-team.ru/cloud/mdb/internal/juggler"
	"a.yandex-team.ru/cloud/mdb/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/library/go/slices"
)

type WaitOtherLegsReachableStep struct {
	reachability juggler.JugglerChecker
	dom0d        dom0discovery.Dom0Discovery
	mDB          metadb.MetaDB
}

func (s *WaitOtherLegsReachableStep) GetStepName() string {
	return "wait other legs reachable"
}

type UnreachableLegsMap struct {
	unreachable map[string][]string
	unknown     []string
}

func (s *WaitOtherLegsReachableStep) checkOtherLegsReachable(ctx context.Context, currentFQDNs []string) (UnreachableLegsMap, error) {
	result := UnreachableLegsMap{
		unreachable: map[string][]string{},
	}
	now := time.Now()

	for _, fqdn := range currentFQDNs {
		var otherLegs []string
		dscvr := metadbdiscovery.NewMetaDBBasedDiscovery(s.mDB)
		neighbours, err := dscvr.FindInShardOrSubcidByFQDN(ctx, fqdn)
		if err != nil {
			if semerr.IsNotFound(err) {
				continue // non dataplane host
			}
			return result, err
		}
		for _, host := range neighbours.Others {
			if host.FQDN != fqdn {
				otherLegs = append(otherLegs, host.FQDN)
			}
		}

		if reachabilityResult, err := s.reachability.Check(ctx, otherLegs, []string{juggler.Unreachable}, now); err != nil {
			return result, err
		} else {
			if len(reachabilityResult.NotOK) > 0 {
				result.unreachable[fqdn] = reachabilityResult.NotOK
			}
		}
	}
	return result, nil
}

func fmtUnreachableLegsMapResult(ulm UnreachableLegsMap) RunResult {
	var msgs []string
	if len(ulm.unknown) > 0 {
		msgs = append(msgs, fmt.Sprintf("%d legs skipped, they are not in metadb, so they have no legs: %s", len(ulm.unknown), slices.Join(ulm.unknown, ", ")))
	}
	if len(ulm.unreachable) > 0 {
		msgs := []string{"NotOK legs exist, you should bring them up:"}
		for fqdn, legs := range ulm.unreachable {
			msgs = append(msgs, fmt.Sprintf("  * for '%s' unreachable: %s", fqdn, slices.Join(legs, ", ")))
		}
		return waitWithMessage(slices.Join(msgs, "\n"))
	}
	msgs = append(msgs, "All known clusters reachable")
	return continueWithMessage(slices.Join(msgs, "\n"))
}

func (s *WaitOtherLegsReachableStep) RunStep(ctx context.Context, execCtx *InstructionCtx) RunResult {
	rd := execCtx.GetActualRD()
	dom0 := rd.R.MustOneFQDN()
	var checkFQDNS []string
	if containers, err := s.dom0d.Dom0Instances(ctx, dom0); err != nil {
		return errorListingDBM(err)
	} else {
		for _, cnt := range containers.WellKnown {
			checkFQDNS = append(checkFQDNS, cnt.FQDN)
		}
	}
	reachableResult, err := s.checkOtherLegsReachable(ctx, checkFQDNS)
	if err != nil {
		return waitWithErrAndMessage(err, "error checking other legs, will try again")
	}
	return fmtUnreachableLegsMapResult(reachableResult)
}

func NewWaitOtherLegsReachableStep(jglr jugglerapi.API, dom0d dom0discovery.Dom0Discovery, mDB metadb.MetaDB, jglrConf juggler.JugglerConfig) DecisionStep {
	return &WaitOtherLegsReachableStep{
		reachability: juggler.NewJugglerReachabilityChecker(jglr, jglrConf.UnreachableServiceWindowMin),
		dom0d:        dom0d,
		mDB:          mDB,
	}
}
