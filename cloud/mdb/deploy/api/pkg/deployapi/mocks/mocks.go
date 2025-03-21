// Code generated by MockGen. DO NOT EDIT.
// Source: a.yandex-team.ru/cloud/mdb/deploy/api/pkg/deployapi (interfaces: Client)

// Package mocks is a generated GoMock package.
package mocks

import (
	deployapi "a.yandex-team.ru/cloud/mdb/deploy/api/pkg/deployapi"
	models "a.yandex-team.ru/cloud/mdb/deploy/api/pkg/models"
	context "context"
	gomock "github.com/golang/mock/gomock"
	reflect "reflect"
	time "time"
)

// MockClient is a mock of Client interface.
type MockClient struct {
	ctrl     *gomock.Controller
	recorder *MockClientMockRecorder
}

// MockClientMockRecorder is the mock recorder for MockClient.
type MockClientMockRecorder struct {
	mock *MockClient
}

// NewMockClient creates a new mock instance.
func NewMockClient(ctrl *gomock.Controller) *MockClient {
	mock := &MockClient{ctrl: ctrl}
	mock.recorder = &MockClientMockRecorder{mock}
	return mock
}

// EXPECT returns an object that allows the caller to indicate expected use.
func (m *MockClient) EXPECT() *MockClientMockRecorder {
	return m.recorder
}

// CreateGroup mocks base method.
func (m *MockClient) CreateGroup(arg0 context.Context, arg1 string) (models.Group, error) {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "CreateGroup", arg0, arg1)
	ret0, _ := ret[0].(models.Group)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// CreateGroup indicates an expected call of CreateGroup.
func (mr *MockClientMockRecorder) CreateGroup(arg0, arg1 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "CreateGroup", reflect.TypeOf((*MockClient)(nil).CreateGroup), arg0, arg1)
}

// CreateJobResult mocks base method.
func (m *MockClient) CreateJobResult(arg0 context.Context, arg1, arg2, arg3 string) (models.JobResult, error) {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "CreateJobResult", arg0, arg1, arg2, arg3)
	ret0, _ := ret[0].(models.JobResult)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// CreateJobResult indicates an expected call of CreateJobResult.
func (mr *MockClientMockRecorder) CreateJobResult(arg0, arg1, arg2, arg3 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "CreateJobResult", reflect.TypeOf((*MockClient)(nil).CreateJobResult), arg0, arg1, arg2, arg3)
}

// CreateMaster mocks base method.
func (m *MockClient) CreateMaster(arg0 context.Context, arg1, arg2 string, arg3 bool, arg4 string) (models.Master, error) {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "CreateMaster", arg0, arg1, arg2, arg3, arg4)
	ret0, _ := ret[0].(models.Master)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// CreateMaster indicates an expected call of CreateMaster.
func (mr *MockClientMockRecorder) CreateMaster(arg0, arg1, arg2, arg3, arg4 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "CreateMaster", reflect.TypeOf((*MockClient)(nil).CreateMaster), arg0, arg1, arg2, arg3, arg4)
}

// CreateMinion mocks base method.
func (m *MockClient) CreateMinion(arg0 context.Context, arg1, arg2 string, arg3 bool) (models.Minion, error) {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "CreateMinion", arg0, arg1, arg2, arg3)
	ret0, _ := ret[0].(models.Minion)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// CreateMinion indicates an expected call of CreateMinion.
func (mr *MockClientMockRecorder) CreateMinion(arg0, arg1, arg2, arg3 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "CreateMinion", reflect.TypeOf((*MockClient)(nil).CreateMinion), arg0, arg1, arg2, arg3)
}

// CreateShipment mocks base method.
func (m *MockClient) CreateShipment(arg0 context.Context, arg1 []string, arg2 []models.CommandDef, arg3, arg4 int64, arg5 time.Duration) (models.Shipment, error) {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "CreateShipment", arg0, arg1, arg2, arg3, arg4, arg5)
	ret0, _ := ret[0].(models.Shipment)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// CreateShipment indicates an expected call of CreateShipment.
func (mr *MockClientMockRecorder) CreateShipment(arg0, arg1, arg2, arg3, arg4, arg5 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "CreateShipment", reflect.TypeOf((*MockClient)(nil).CreateShipment), arg0, arg1, arg2, arg3, arg4, arg5)
}

// DeleteMinion mocks base method.
func (m *MockClient) DeleteMinion(arg0 context.Context, arg1 string) error {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "DeleteMinion", arg0, arg1)
	ret0, _ := ret[0].(error)
	return ret0
}

// DeleteMinion indicates an expected call of DeleteMinion.
func (mr *MockClientMockRecorder) DeleteMinion(arg0, arg1 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "DeleteMinion", reflect.TypeOf((*MockClient)(nil).DeleteMinion), arg0, arg1)
}

// GetCommand mocks base method.
func (m *MockClient) GetCommand(arg0 context.Context, arg1 models.CommandID) (models.Command, error) {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "GetCommand", arg0, arg1)
	ret0, _ := ret[0].(models.Command)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// GetCommand indicates an expected call of GetCommand.
func (mr *MockClientMockRecorder) GetCommand(arg0, arg1 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "GetCommand", reflect.TypeOf((*MockClient)(nil).GetCommand), arg0, arg1)
}

// GetCommands mocks base method.
func (m *MockClient) GetCommands(arg0 context.Context, arg1 deployapi.SelectCommandsAttrs, arg2 deployapi.Paging) ([]models.Command, deployapi.Paging, error) {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "GetCommands", arg0, arg1, arg2)
	ret0, _ := ret[0].([]models.Command)
	ret1, _ := ret[1].(deployapi.Paging)
	ret2, _ := ret[2].(error)
	return ret0, ret1, ret2
}

// GetCommands indicates an expected call of GetCommands.
func (mr *MockClientMockRecorder) GetCommands(arg0, arg1, arg2 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "GetCommands", reflect.TypeOf((*MockClient)(nil).GetCommands), arg0, arg1, arg2)
}

// GetGroup mocks base method.
func (m *MockClient) GetGroup(arg0 context.Context, arg1 string) (models.Group, error) {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "GetGroup", arg0, arg1)
	ret0, _ := ret[0].(models.Group)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// GetGroup indicates an expected call of GetGroup.
func (mr *MockClientMockRecorder) GetGroup(arg0, arg1 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "GetGroup", reflect.TypeOf((*MockClient)(nil).GetGroup), arg0, arg1)
}

// GetGroups mocks base method.
func (m *MockClient) GetGroups(arg0 context.Context, arg1 deployapi.Paging) ([]models.Group, deployapi.Paging, error) {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "GetGroups", arg0, arg1)
	ret0, _ := ret[0].([]models.Group)
	ret1, _ := ret[1].(deployapi.Paging)
	ret2, _ := ret[2].(error)
	return ret0, ret1, ret2
}

// GetGroups indicates an expected call of GetGroups.
func (mr *MockClientMockRecorder) GetGroups(arg0, arg1 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "GetGroups", reflect.TypeOf((*MockClient)(nil).GetGroups), arg0, arg1)
}

// GetJob mocks base method.
func (m *MockClient) GetJob(arg0 context.Context, arg1 models.JobID) (models.Job, error) {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "GetJob", arg0, arg1)
	ret0, _ := ret[0].(models.Job)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// GetJob indicates an expected call of GetJob.
func (mr *MockClientMockRecorder) GetJob(arg0, arg1 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "GetJob", reflect.TypeOf((*MockClient)(nil).GetJob), arg0, arg1)
}

// GetJobResult mocks base method.
func (m *MockClient) GetJobResult(arg0 context.Context, arg1 models.JobResultID) (models.JobResult, error) {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "GetJobResult", arg0, arg1)
	ret0, _ := ret[0].(models.JobResult)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// GetJobResult indicates an expected call of GetJobResult.
func (mr *MockClientMockRecorder) GetJobResult(arg0, arg1 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "GetJobResult", reflect.TypeOf((*MockClient)(nil).GetJobResult), arg0, arg1)
}

// GetJobResults mocks base method.
func (m *MockClient) GetJobResults(arg0 context.Context, arg1 deployapi.SelectJobResultsAttrs, arg2 deployapi.Paging) ([]models.JobResult, deployapi.Paging, error) {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "GetJobResults", arg0, arg1, arg2)
	ret0, _ := ret[0].([]models.JobResult)
	ret1, _ := ret[1].(deployapi.Paging)
	ret2, _ := ret[2].(error)
	return ret0, ret1, ret2
}

// GetJobResults indicates an expected call of GetJobResults.
func (mr *MockClientMockRecorder) GetJobResults(arg0, arg1, arg2 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "GetJobResults", reflect.TypeOf((*MockClient)(nil).GetJobResults), arg0, arg1, arg2)
}

// GetJobs mocks base method.
func (m *MockClient) GetJobs(arg0 context.Context, arg1 deployapi.SelectJobsAttrs, arg2 deployapi.Paging) ([]models.Job, deployapi.Paging, error) {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "GetJobs", arg0, arg1, arg2)
	ret0, _ := ret[0].([]models.Job)
	ret1, _ := ret[1].(deployapi.Paging)
	ret2, _ := ret[2].(error)
	return ret0, ret1, ret2
}

// GetJobs indicates an expected call of GetJobs.
func (mr *MockClientMockRecorder) GetJobs(arg0, arg1, arg2 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "GetJobs", reflect.TypeOf((*MockClient)(nil).GetJobs), arg0, arg1, arg2)
}

// GetMaster mocks base method.
func (m *MockClient) GetMaster(arg0 context.Context, arg1 string) (models.Master, error) {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "GetMaster", arg0, arg1)
	ret0, _ := ret[0].(models.Master)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// GetMaster indicates an expected call of GetMaster.
func (mr *MockClientMockRecorder) GetMaster(arg0, arg1 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "GetMaster", reflect.TypeOf((*MockClient)(nil).GetMaster), arg0, arg1)
}

// GetMasters mocks base method.
func (m *MockClient) GetMasters(arg0 context.Context, arg1 deployapi.Paging) ([]models.Master, deployapi.Paging, error) {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "GetMasters", arg0, arg1)
	ret0, _ := ret[0].([]models.Master)
	ret1, _ := ret[1].(deployapi.Paging)
	ret2, _ := ret[2].(error)
	return ret0, ret1, ret2
}

// GetMasters indicates an expected call of GetMasters.
func (mr *MockClientMockRecorder) GetMasters(arg0, arg1 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "GetMasters", reflect.TypeOf((*MockClient)(nil).GetMasters), arg0, arg1)
}

// GetMinion mocks base method.
func (m *MockClient) GetMinion(arg0 context.Context, arg1 string) (models.Minion, error) {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "GetMinion", arg0, arg1)
	ret0, _ := ret[0].(models.Minion)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// GetMinion indicates an expected call of GetMinion.
func (mr *MockClientMockRecorder) GetMinion(arg0, arg1 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "GetMinion", reflect.TypeOf((*MockClient)(nil).GetMinion), arg0, arg1)
}

// GetMinionMaster mocks base method.
func (m *MockClient) GetMinionMaster(arg0 context.Context, arg1 string) (deployapi.MinionMaster, error) {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "GetMinionMaster", arg0, arg1)
	ret0, _ := ret[0].(deployapi.MinionMaster)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// GetMinionMaster indicates an expected call of GetMinionMaster.
func (mr *MockClientMockRecorder) GetMinionMaster(arg0, arg1 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "GetMinionMaster", reflect.TypeOf((*MockClient)(nil).GetMinionMaster), arg0, arg1)
}

// GetMinions mocks base method.
func (m *MockClient) GetMinions(arg0 context.Context, arg1 deployapi.Paging) ([]models.Minion, deployapi.Paging, error) {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "GetMinions", arg0, arg1)
	ret0, _ := ret[0].([]models.Minion)
	ret1, _ := ret[1].(deployapi.Paging)
	ret2, _ := ret[2].(error)
	return ret0, ret1, ret2
}

// GetMinions indicates an expected call of GetMinions.
func (mr *MockClientMockRecorder) GetMinions(arg0, arg1 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "GetMinions", reflect.TypeOf((*MockClient)(nil).GetMinions), arg0, arg1)
}

// GetMinionsByMaster mocks base method.
func (m *MockClient) GetMinionsByMaster(arg0 context.Context, arg1 string, arg2 deployapi.Paging) ([]models.Minion, deployapi.Paging, error) {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "GetMinionsByMaster", arg0, arg1, arg2)
	ret0, _ := ret[0].([]models.Minion)
	ret1, _ := ret[1].(deployapi.Paging)
	ret2, _ := ret[2].(error)
	return ret0, ret1, ret2
}

// GetMinionsByMaster indicates an expected call of GetMinionsByMaster.
func (mr *MockClientMockRecorder) GetMinionsByMaster(arg0, arg1, arg2 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "GetMinionsByMaster", reflect.TypeOf((*MockClient)(nil).GetMinionsByMaster), arg0, arg1, arg2)
}

// GetShipment mocks base method.
func (m *MockClient) GetShipment(arg0 context.Context, arg1 models.ShipmentID) (models.Shipment, error) {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "GetShipment", arg0, arg1)
	ret0, _ := ret[0].(models.Shipment)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// GetShipment indicates an expected call of GetShipment.
func (mr *MockClientMockRecorder) GetShipment(arg0, arg1 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "GetShipment", reflect.TypeOf((*MockClient)(nil).GetShipment), arg0, arg1)
}

// GetShipments mocks base method.
func (m *MockClient) GetShipments(arg0 context.Context, arg1 deployapi.SelectShipmentsAttrs, arg2 deployapi.Paging) ([]models.Shipment, deployapi.Paging, error) {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "GetShipments", arg0, arg1, arg2)
	ret0, _ := ret[0].([]models.Shipment)
	ret1, _ := ret[1].(deployapi.Paging)
	ret2, _ := ret[2].(error)
	return ret0, ret1, ret2
}

// GetShipments indicates an expected call of GetShipments.
func (mr *MockClientMockRecorder) GetShipments(arg0, arg1, arg2 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "GetShipments", reflect.TypeOf((*MockClient)(nil).GetShipments), arg0, arg1, arg2)
}

// IsReady mocks base method.
func (m *MockClient) IsReady(arg0 context.Context) error {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "IsReady", arg0)
	ret0, _ := ret[0].(error)
	return ret0
}

// IsReady indicates an expected call of IsReady.
func (mr *MockClientMockRecorder) IsReady(arg0 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "IsReady", reflect.TypeOf((*MockClient)(nil).IsReady), arg0)
}

// RegisterMinion mocks base method.
func (m *MockClient) RegisterMinion(arg0 context.Context, arg1, arg2 string) (models.Minion, error) {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "RegisterMinion", arg0, arg1, arg2)
	ret0, _ := ret[0].(models.Minion)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// RegisterMinion indicates an expected call of RegisterMinion.
func (mr *MockClientMockRecorder) RegisterMinion(arg0, arg1, arg2 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "RegisterMinion", reflect.TypeOf((*MockClient)(nil).RegisterMinion), arg0, arg1, arg2)
}

// UnregisterMinion mocks base method.
func (m *MockClient) UnregisterMinion(arg0 context.Context, arg1 string) (models.Minion, error) {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "UnregisterMinion", arg0, arg1)
	ret0, _ := ret[0].(models.Minion)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// UnregisterMinion indicates an expected call of UnregisterMinion.
func (mr *MockClientMockRecorder) UnregisterMinion(arg0, arg1 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "UnregisterMinion", reflect.TypeOf((*MockClient)(nil).UnregisterMinion), arg0, arg1)
}

// UpsertMaster mocks base method.
func (m *MockClient) UpsertMaster(arg0 context.Context, arg1 string, arg2 deployapi.UpsertMasterAttrs) (models.Master, error) {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "UpsertMaster", arg0, arg1, arg2)
	ret0, _ := ret[0].(models.Master)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// UpsertMaster indicates an expected call of UpsertMaster.
func (mr *MockClientMockRecorder) UpsertMaster(arg0, arg1, arg2 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "UpsertMaster", reflect.TypeOf((*MockClient)(nil).UpsertMaster), arg0, arg1, arg2)
}

// UpsertMinion mocks base method.
func (m *MockClient) UpsertMinion(arg0 context.Context, arg1 string, arg2 deployapi.UpsertMinionAttrs) (models.Minion, error) {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "UpsertMinion", arg0, arg1, arg2)
	ret0, _ := ret[0].(models.Minion)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// UpsertMinion indicates an expected call of UpsertMinion.
func (mr *MockClientMockRecorder) UpsertMinion(arg0, arg1, arg2 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "UpsertMinion", reflect.TypeOf((*MockClient)(nil).UpsertMinion), arg0, arg1, arg2)
}
