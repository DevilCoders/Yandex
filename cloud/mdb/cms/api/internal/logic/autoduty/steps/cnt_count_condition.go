package steps

import (
	"context"
	"strings"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/dom0discovery"
)

type ContainerCountConditionStep struct {
	dom0   dom0discovery.Dom0Discovery
	handle func(result dom0discovery.DiscoveryResult) RunResult
}

func (s *ContainerCountConditionStep) GetStepName() string {
	return "count containers"
}

func (s *ContainerCountConditionStep) RunStep(ctx context.Context, execCtx *InstructionCtx) RunResult {
	rd := execCtx.GetActualRD()
	cntrs, err := s.dom0.Dom0Instances(ctx, rd.R.MustOneFQDN())
	if err != nil {
		return errorListingDBM(err)
	}

	if len(cntrs.Unknown) > 0 {
		return waitWithMessage(
			"Some containers are not registered. You should ensure conductor group for them exists, and add to blacklist or to whitelist in cms pillar: %s",
			strings.Join(cntrs.Unknown, ", "),
		)
	}

	return s.handle(cntrs)
}

func NewMetadataNeededForNonempty(dom0 dom0discovery.Dom0Discovery, onNonEmpty func() []DecisionStep) DecisionStep {
	return &ContainerCountConditionStep{
		dom0: dom0,
		handle: func(result dom0discovery.DiscoveryResult) RunResult {
			if len(result.WellKnown) > 0 {
				return continueWithMessage("with containers, must bring fresh metadata", onNonEmpty()...)
			}
			return continueWithMessage("0")
		},
	}
}

func NewEmptyDom0Step(dom0 dom0discovery.Dom0Discovery) DecisionStep {
	return &ContainerCountConditionStep{
		dom0: dom0,
		handle: func(result dom0discovery.DiscoveryResult) RunResult {
			if len(result.WellKnown) == 0 {
				return approveWithMessage("0")
			}
			return continueWithMessage("with containers")
		},
	}
}
