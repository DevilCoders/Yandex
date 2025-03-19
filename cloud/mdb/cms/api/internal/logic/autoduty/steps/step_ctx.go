package steps

import (
	"a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/types"
	"a.yandex-team.ru/cloud/mdb/cms/api/pkg/models"
)

type InstructionCtx struct {
	// RequestDecisionTuple in progress
	rd *types.RequestDecisionTuple
	// Requests visible to Wall-e - non deleted yet.
	// may be handled right now, may be not, we don't know.
	walleSees []models.ManagementRequest
	// AutoDuty has already processed these RequestDecisionTuples
	doneRDs []types.RequestDecisionTuple
}

func (ic *InstructionCtx) SetActualRD(rd *types.RequestDecisionTuple) {
	if ic.rd != nil {
		ic.doneRDs = append(ic.doneRDs, *ic.rd)
	}
	ic.rd = rd
}

func (ic *InstructionCtx) GetActualRD() *types.RequestDecisionTuple {
	return ic.rd
}

// Requests we are going to let go in this run of autoduty
func (ic *InstructionCtx) DoneRDs() []types.RequestDecisionTuple {
	return ic.doneRDs
}

func (ic *InstructionCtx) TouchedCount() int {
	result := len(ic.doneRDs)
	if ic.GetActualRD() != nil {
		result += 1
	}
	return result
}

// Requests we let go in previous runs of autoduty, and not yet returned
func (ic *InstructionCtx) RequestsGivenAwayBeforeThisCtx() []models.ManagementRequest {
	var result []models.ManagementRequest
	for _, r := range ic.walleSees {
		if r.Status.IsGivenAway() {
			result = append(result, r)
		}
	}
	return result
}

// Returns all requests we do not control, already at Wall-e.
// INCLUDING current run of autoduty
func (ic *InstructionCtx) RequestsGivenAway() []models.ManagementRequest {
	reqs := ic.RequestsGivenAwayBeforeThisCtx()
	// requests from CURRENT run of autoduty
	for _, rd := range ic.DoneRDs() {
		if rd.R.Status.IsGivenAway() {
			reqs = append(reqs, rd.R)
		}
	}
	return reqs
}

func NewInstructionCtx(reqs []models.ManagementRequest) InstructionCtx {
	return InstructionCtx{
		rd:        nil,
		walleSees: reqs,
		doneRDs:   []types.RequestDecisionTuple{},
	}
}

func NewEmptyInstructionCtx() InstructionCtx {
	return NewInstructionCtx([]models.ManagementRequest{})
}
