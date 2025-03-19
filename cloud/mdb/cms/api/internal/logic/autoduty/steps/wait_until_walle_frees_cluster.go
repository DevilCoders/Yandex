package steps

import (
	"context"
	"fmt"
	"strings"
	"time"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/dom0discovery"
	cms_models "a.yandex-team.ru/cloud/mdb/cms/api/internal/models"
	"a.yandex-team.ru/cloud/mdb/cms/api/pkg/models"
	"a.yandex-team.ru/library/go/slices"
)

type WaitUntilWalleFreesCluster struct {
	dom0 dom0discovery.Dom0Discovery
}

func (s *WaitUntilWalleFreesCluster) GetStepName() string {
	return "all clusters are not at Wall-e"
}

func errorListingDBM(err error) RunResult {
	return waitWithErrAndMessage(err, "could not get list of containers on dom0")
}

func DBMContainersToClusterNames(l []cms_models.Instance) []string {
	var result []string
	for _, cnt := range l {
		result = append(result, cnt.DBMClusterName)
	}
	return result
}

func (s *WaitUntilWalleFreesCluster) WaitIfAnyClusterBusy(ctx context.Context, localClusterNames []string, r models.ManagementRequest) *RunResult {
	if !r.MayBeTakenByWalle() {
		return nil // TODO: тест на то что это меняется в контексте в Transit
	}
	containers, err := s.dom0.Dom0Instances(ctx, r.MustOneFQDN())
	if err != nil {
		rr := errorListingDBM(err)
		return &rr
	}
	for _, cn := range containers.WellKnown {
		if slices.ContainsString(localClusterNames, cn.DBMClusterName) {
			rr := waitWithMessage(
				"wait until Wall-e returns cluster '%s'. We gave it %s ago.",
				cn.DBMClusterName, time.Since(r.ResolvedAt).Round(time.Second))
			return &rr
		}
	}
	return nil
}

func (s *WaitUntilWalleFreesCluster) RunStep(ctx context.Context, execCtx *InstructionCtx) RunResult {
	// get cluster which live on this dom0
	rd := execCtx.GetActualRD()
	containers, err := s.dom0.Dom0Instances(ctx, rd.R.MustOneFQDN())
	if err != nil {
		return errorListingDBM(err)
	}
	localClusterNames := DBMContainersToClusterNames(containers.WellKnown)

	for _, r := range execCtx.RequestsGivenAway() {
		rr := s.WaitIfAnyClusterBusy(ctx, localClusterNames, r)
		if rr != nil {
			return *rr
		}
	}

	return continueWithMessage(fmt.Sprintf("wall-e does not currently perform any operations on any host of these %d clusters: %s",
		len(localClusterNames), strings.Join(localClusterNames, ", ")))
}

func NewWaitUntilWalleFreesCluster(dom0 dom0discovery.Dom0Discovery) DecisionStep {
	return &WaitUntilWalleFreesCluster{dom0: dom0}
}
