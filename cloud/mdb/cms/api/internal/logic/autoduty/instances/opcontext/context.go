package opcontext

import (
	"a.yandex-team.ru/cloud/mdb/cms/api/pkg/models"
)

type OperationContext struct {
	currentOperation models.ManagementInstanceOperation
	doneOperations   []models.ManagementInstanceOperation
}

func NewStepContext(op models.ManagementInstanceOperation) *OperationContext {
	return &OperationContext{
		currentOperation: op,
	}
}

func (s *OperationContext) InstanceID() string {
	return s.currentOperation.InstanceID
}

func (s *OperationContext) OperationID() string {
	return s.currentOperation.ID
}

func (s *OperationContext) OperationType() models.InstanceOperationType {
	return s.currentOperation.Type
}

func (s *OperationContext) SetLog(log string) {
	s.currentOperation.Log = log
}

func (s *OperationContext) setStatus(status models.InstanceOperationStatus) {
	s.currentOperation.Status = status
}

func (s *OperationContext) GetStatus() models.InstanceOperationStatus {
	return s.currentOperation.Status
}

func (s *OperationContext) SetInProgress() {
	s.setStatus(models.InstanceOperationStatusInProgress)
}

func (s *OperationContext) SetOk() {
	s.setStatus(models.InstanceOperationStatusOK)
}

func (s *OperationContext) SetRejected() {
	s.setStatus(models.InstanceOperationStatusRejected)
}

func (s *OperationContext) SetOKPending() {
	s.setStatus(models.InstanceOperationStatusOkPending)
}

func (s *OperationContext) SetRejectPending() {
	s.setStatus(models.InstanceOperationStatusRejectPending)
}

func (s *OperationContext) SetExplanation(e string) {
	s.currentOperation.Explanation = e
}

func (s *OperationContext) SetStepNames(steps []string) {
	s.currentOperation.ExecutedStepNames = steps
}

func (s *OperationContext) State() *models.OperationState {
	return s.currentOperation.State
}

func (s *OperationContext) CurrentOperation() models.ManagementInstanceOperation {
	return s.currentOperation
}

func (s *OperationContext) FQDN() string {
	return s.State().FQDN
}

func (s *OperationContext) SetFQDN(fqdn string) *OperationContext {
	s.State().FQDN = fqdn
	return s
}

func (s *OperationContext) Dom0FQDN() string {
	return s.State().Dom0FQDN
}

func (s *OperationContext) SetDom0FQDN(fqdn string) *OperationContext {
	s.State().Dom0FQDN = fqdn
	return s
}
