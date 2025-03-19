package models

import (
	"time"

	"a.yandex-team.ru/cloud/mdb/cms/api/internal/lockcluster"
	deploymodels "a.yandex-team.ru/cloud/mdb/deploy/api/pkg/models"
)

type InstanceOperationType string
type InstanceOperationStatus string

type ManagementInstanceOperation struct {
	ExecutedStepNames []string
	ID                string
	Type              InstanceOperationType
	Status            InstanceOperationStatus
	Comment           string
	Author            Person
	InstanceID        string
	CreatedAt         time.Time
	ModifiedAt        time.Time
	Explanation       string
	Log               string
	State             *OperationState
}

const (
	InstanceOperationMove            InstanceOperationType = "move"
	InstanceOperationWhipPrimaryAway InstanceOperationType = "whip_primary_away"

	InstanceOperationStatusNew           InstanceOperationStatus = "new"
	InstanceOperationStatusInProgress    InstanceOperationStatus = "in-progress"
	InstanceOperationStatusOK            InstanceOperationStatus = "ok"
	InstanceOperationStatusRejected      InstanceOperationStatus = "rejected"
	InstanceOperationStatusOkPending     InstanceOperationStatus = "ok-pending"
	InstanceOperationStatusRejectPending InstanceOperationStatus = "reject-pending"
)

type OperationState struct {
	MoveInstanceStep   *MoveInstanceStepState   `json:"move_instance_step" yaml:"move_instance_step"`
	DowntimesStep      *DowntimesStepState      `json:"downtimes_step" yaml:"downtimes_step"`
	FQDN               string                   `json:"fqdn" yaml:"fqdn"`
	Dom0FQDN           string                   `json:"dom0_fqdn" yaml:"dom0_fqdn"`
	WaitForHealthyStep *WaitForHealthyStepState `json:"wait_for_healthy_step" yaml:"wait_for_healthy_step"`
	WhipMasterStep     *WhipMasterStepState     `json:"whip_master_step" yaml:"whip_master_step"`
	Lock               *lockcluster.State       `json:"lock" yaml:"lock"`
	CheckIsMasterStep  *CheckIsMasterStepState  `json:"check_is_master_step" yaml:"check_is_master_step"`
}

type MoveInstanceStepState struct {
	TaskID string `json:"task_id" yaml:"task_id"`
}

type DowntimesStepState struct {
	DowntimeIDs []string `json:"downtime_ids"`
}

type WaitForHealthyStepState struct {
	Checked bool `json:"checked" yaml:"checked"`
}

type WhipMasterStepState struct {
	Shipment deploymodels.ShipmentID `json:"shipments" yaml:"shipments"`
}

type CheckIsMasterStepState struct {
	IsNotMaster bool   `json:"is_not_master" yaml:"is_not_master"`
	Reason      string `json:"reason" yaml:"reason"`
}

type LockState struct {
	LockID string `json:"lock_id" yaml:"lock_id"`
}

func DefaultMoveInstanceStepState() *MoveInstanceStepState {
	return &MoveInstanceStepState{
		TaskID: "",
	}
}

func DefaultDowntimesStepState() *DowntimesStepState {
	return &DowntimesStepState{}
}

func DefaultWaitForHealthyStepState() *WaitForHealthyStepState {
	return &WaitForHealthyStepState{}
}

func DefaultWhipMasterStepState() *WhipMasterStepState {
	return &WhipMasterStepState{
		Shipment: 0,
	}
}

func DefaultLockState() *lockcluster.State {
	return &lockcluster.State{}
}

func DefaultCheckIsMasterStepState() *CheckIsMasterStepState {
	return &CheckIsMasterStepState{}
}

func DefaultOperationState() *OperationState {
	return &OperationState{
		MoveInstanceStep:   DefaultMoveInstanceStepState(),
		DowntimesStep:      DefaultDowntimesStepState(),
		FQDN:               "",
		Dom0FQDN:           "",
		WaitForHealthyStep: DefaultWaitForHealthyStepState(),
		WhipMasterStep:     DefaultWhipMasterStepState(),
		Lock:               DefaultLockState(),
		CheckIsMasterStep:  DefaultCheckIsMasterStepState(),
	}
}

func (o *OperationState) SetMoveInstanceStepState(s *MoveInstanceStepState) *OperationState {
	o.MoveInstanceStep = s
	return o
}

func (o *OperationState) SetDowntimesStepState(s *DowntimesStepState) *OperationState {
	o.DowntimesStep = s
	return o
}

func (o *OperationState) SetFQDN(s string) *OperationState {
	o.FQDN = s
	return o
}

func (o *OperationState) SetDom0FQDN(s string) *OperationState {
	o.Dom0FQDN = s
	return o
}

func (o *OperationState) SetWaitForHealthyStep(s *WaitForHealthyStepState) *OperationState {
	o.WaitForHealthyStep = s
	return o
}

func (o *OperationState) SetWhipMasterStepState(s *WhipMasterStepState) *OperationState {
	o.WhipMasterStep = s
	return o
}

func (o *OperationState) SetLockState(s *lockcluster.State) *OperationState {
	o.Lock = s
	return o
}

func (o *OperationState) SetCheckIsMasterStepState(s *CheckIsMasterStepState) *OperationState {
	o.CheckIsMasterStep = s
	return o
}
