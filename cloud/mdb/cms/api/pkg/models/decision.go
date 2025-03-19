package models

import (
	"strings"
	"time"

	"a.yandex-team.ru/library/go/core/xerrors"
)

type DecisionStatus string
type AutoResolution string

const (
	DecisionNone       DecisionStatus = "new"
	DecisionProcessing DecisionStatus = "processing"
	DecisionWait       DecisionStatus = "wait"
	DecisionApprove    DecisionStatus = "ok"
	DecisionReject     DecisionStatus = "rejected"
	DecisionAtWalle    DecisionStatus = "at-wall-e"
	DecisionBeforeDone DecisionStatus = "before-done"
	DecisionEscalate   DecisionStatus = "escalated"
	DecisionDone       DecisionStatus = "done"
	DecisionCleanup    DecisionStatus = "cleanup"
)

var KnownDecisionMap = map[DecisionStatus]struct{}{
	DecisionNone:       {},
	DecisionProcessing: {},
	DecisionWait:       {},
	DecisionApprove:    {},
	DecisionAtWalle:    {},
	DecisionEscalate:   {},
	DecisionBeforeDone: {},
	DecisionDone:       {},
	DecisionCleanup:    {},
}

const (
	ResolutionUnknown AutoResolution = "unknown"
	ResolutionApprove AutoResolution = "approved"
)

type AutomaticDecision struct {
	ID            int64
	RequestID     int64
	Status        DecisionStatus // life cycle status of Decision
	Resolution    AutoResolution // auto duty has made this resolution after analysis
	AnalysisLog   string
	MutationsLog  string
	AfterWalleLog string
	CleanupLog    string
	DecidedAt     time.Time
	OpsLog        *OpsMetaLog
}

var KnownResolutionMap = map[AutoResolution]struct{}{
	ResolutionUnknown: {},
	ResolutionApprove: {},
}

func ParseAutomaticDecision(s string) (DecisionStatus, error) {
	d := DecisionStatus(strings.ToLower(s))
	if _, ok := KnownDecisionMap[d]; !ok {
		return d, xerrors.Errorf("unknown status in db %q", s)
	}
	return d, nil
}

func ParseAutomaticResolution(s string) (AutoResolution, error) {
	d := AutoResolution(strings.ToLower(s))
	if _, ok := KnownResolutionMap[d]; !ok {
		return d, xerrors.Errorf("resolution in db %s", s)
	}
	return d, nil
}

var AutoDecisionStatusMap = map[DecisionStatus]RequestStatus{
	DecisionApprove: StatusOK,
	DecisionReject:  StatusRejected,
}
