// Code generated by MockGen. DO NOT EDIT.
// Source: a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/postgresql (interfaces: PostgreSQL,PerfDiag)

// Package mocks is a generated GoMock package.
package mocks

import (
	compute "a.yandex-team.ru/cloud/mdb/internal/compute/compute"
	pgmodels "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/postgresql/pgmodels"
	console "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/console"
	context "context"
	gomock "github.com/golang/mock/gomock"
	reflect "reflect"
)

// MockPostgreSQL is a mock of PostgreSQL interface.
type MockPostgreSQL struct {
	ctrl     *gomock.Controller
	recorder *MockPostgreSQLMockRecorder
}

// MockPostgreSQLMockRecorder is the mock recorder for MockPostgreSQL.
type MockPostgreSQLMockRecorder struct {
	mock *MockPostgreSQL
}

// NewMockPostgreSQL creates a new mock instance.
func NewMockPostgreSQL(ctrl *gomock.Controller) *MockPostgreSQL {
	mock := &MockPostgreSQL{ctrl: ctrl}
	mock.recorder = &MockPostgreSQLMockRecorder{mock}
	return mock
}

// EXPECT returns an object that allows the caller to indicate expected use.
func (m *MockPostgreSQL) EXPECT() *MockPostgreSQLMockRecorder {
	return m.recorder
}

// Database mocks base method.
func (m *MockPostgreSQL) Database(arg0 context.Context, arg1, arg2 string) (pgmodels.Database, error) {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "Database", arg0, arg1, arg2)
	ret0, _ := ret[0].(pgmodels.Database)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// Database indicates an expected call of Database.
func (mr *MockPostgreSQLMockRecorder) Database(arg0, arg1, arg2 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "Database", reflect.TypeOf((*MockPostgreSQL)(nil).Database), arg0, arg1, arg2)
}

// Databases mocks base method.
func (m *MockPostgreSQL) Databases(arg0 context.Context, arg1 string, arg2, arg3 int64) ([]pgmodels.Database, error) {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "Databases", arg0, arg1, arg2, arg3)
	ret0, _ := ret[0].([]pgmodels.Database)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// Databases indicates an expected call of Databases.
func (mr *MockPostgreSQLMockRecorder) Databases(arg0, arg1, arg2, arg3 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "Databases", reflect.TypeOf((*MockPostgreSQL)(nil).Databases), arg0, arg1, arg2, arg3)
}

// GetDefaultVersions mocks base method.
func (m *MockPostgreSQL) GetDefaultVersions(arg0 context.Context) ([]console.DefaultVersion, error) {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "GetDefaultVersions", arg0)
	ret0, _ := ret[0].([]console.DefaultVersion)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// GetDefaultVersions indicates an expected call of GetDefaultVersions.
func (mr *MockPostgreSQLMockRecorder) GetDefaultVersions(arg0 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "GetDefaultVersions", reflect.TypeOf((*MockPostgreSQL)(nil).GetDefaultVersions), arg0)
}

// GetHostGroupType mocks base method.
func (m *MockPostgreSQL) GetHostGroupType(arg0 context.Context, arg1 []string) (map[string]compute.HostGroupHostType, error) {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "GetHostGroupType", arg0, arg1)
	ret0, _ := ret[0].(map[string]compute.HostGroupHostType)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// GetHostGroupType indicates an expected call of GetHostGroupType.
func (mr *MockPostgreSQLMockRecorder) GetHostGroupType(arg0, arg1 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "GetHostGroupType", reflect.TypeOf((*MockPostgreSQL)(nil).GetHostGroupType), arg0, arg1)
}

// MockPerfDiag is a mock of PerfDiag interface.
type MockPerfDiag struct {
	ctrl     *gomock.Controller
	recorder *MockPerfDiagMockRecorder
}

// MockPerfDiagMockRecorder is the mock recorder for MockPerfDiag.
type MockPerfDiagMockRecorder struct {
	mock *MockPerfDiag
}

// NewMockPerfDiag creates a new mock instance.
func NewMockPerfDiag(ctrl *gomock.Controller) *MockPerfDiag {
	mock := &MockPerfDiag{ctrl: ctrl}
	mock.recorder = &MockPerfDiagMockRecorder{mock}
	return mock
}

// EXPECT returns an object that allows the caller to indicate expected use.
func (m *MockPerfDiag) EXPECT() *MockPerfDiagMockRecorder {
	return m.recorder
}

// GetSessionsAtTime mocks base method.
func (m *MockPerfDiag) GetSessionsAtTime(arg0 context.Context, arg1 string, arg2 int64, arg3 pgmodels.GetSessionsAtTimeOptions) (pgmodels.SessionsAtTime, bool, error) {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "GetSessionsAtTime", arg0, arg1, arg2, arg3)
	ret0, _ := ret[0].(pgmodels.SessionsAtTime)
	ret1, _ := ret[1].(bool)
	ret2, _ := ret[2].(error)
	return ret0, ret1, ret2
}

// GetSessionsAtTime indicates an expected call of GetSessionsAtTime.
func (mr *MockPerfDiagMockRecorder) GetSessionsAtTime(arg0, arg1, arg2, arg3 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "GetSessionsAtTime", reflect.TypeOf((*MockPerfDiag)(nil).GetSessionsAtTime), arg0, arg1, arg2, arg3)
}

// GetSessionsStats mocks base method.
func (m *MockPerfDiag) GetSessionsStats(arg0 context.Context, arg1 string, arg2 int64, arg3 pgmodels.GetSessionsStatsOptions) ([]pgmodels.SessionsStats, bool, error) {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "GetSessionsStats", arg0, arg1, arg2, arg3)
	ret0, _ := ret[0].([]pgmodels.SessionsStats)
	ret1, _ := ret[1].(bool)
	ret2, _ := ret[2].(error)
	return ret0, ret1, ret2
}

// GetSessionsStats indicates an expected call of GetSessionsStats.
func (mr *MockPerfDiagMockRecorder) GetSessionsStats(arg0, arg1, arg2, arg3 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "GetSessionsStats", reflect.TypeOf((*MockPerfDiag)(nil).GetSessionsStats), arg0, arg1, arg2, arg3)
}

// GetStatementStats mocks base method.
func (m *MockPerfDiag) GetStatementStats(arg0 context.Context, arg1 string, arg2 int64, arg3 pgmodels.GetStatementStatsOptions) (pgmodels.StatementStats, bool, error) {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "GetStatementStats", arg0, arg1, arg2, arg3)
	ret0, _ := ret[0].(pgmodels.StatementStats)
	ret1, _ := ret[1].(bool)
	ret2, _ := ret[2].(error)
	return ret0, ret1, ret2
}

// GetStatementStats indicates an expected call of GetStatementStats.
func (mr *MockPerfDiagMockRecorder) GetStatementStats(arg0, arg1, arg2, arg3 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "GetStatementStats", reflect.TypeOf((*MockPerfDiag)(nil).GetStatementStats), arg0, arg1, arg2, arg3)
}

// GetStatementsAtTime mocks base method.
func (m *MockPerfDiag) GetStatementsAtTime(arg0 context.Context, arg1 string, arg2 int64, arg3 pgmodels.GetStatementsAtTimeOptions) (pgmodels.StatementsAtTime, bool, error) {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "GetStatementsAtTime", arg0, arg1, arg2, arg3)
	ret0, _ := ret[0].(pgmodels.StatementsAtTime)
	ret1, _ := ret[1].(bool)
	ret2, _ := ret[2].(error)
	return ret0, ret1, ret2
}

// GetStatementsAtTime indicates an expected call of GetStatementsAtTime.
func (mr *MockPerfDiagMockRecorder) GetStatementsAtTime(arg0, arg1, arg2, arg3 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "GetStatementsAtTime", reflect.TypeOf((*MockPerfDiag)(nil).GetStatementsAtTime), arg0, arg1, arg2, arg3)
}

// GetStatementsDiff mocks base method.
func (m *MockPerfDiag) GetStatementsDiff(arg0 context.Context, arg1 string, arg2 int64, arg3 pgmodels.GetStatementsDiffOptions) (pgmodels.DiffStatements, bool, error) {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "GetStatementsDiff", arg0, arg1, arg2, arg3)
	ret0, _ := ret[0].(pgmodels.DiffStatements)
	ret1, _ := ret[1].(bool)
	ret2, _ := ret[2].(error)
	return ret0, ret1, ret2
}

// GetStatementsDiff indicates an expected call of GetStatementsDiff.
func (mr *MockPerfDiagMockRecorder) GetStatementsDiff(arg0, arg1, arg2, arg3 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "GetStatementsDiff", reflect.TypeOf((*MockPerfDiag)(nil).GetStatementsDiff), arg0, arg1, arg2, arg3)
}

// GetStatementsInterval mocks base method.
func (m *MockPerfDiag) GetStatementsInterval(arg0 context.Context, arg1 string, arg2 int64, arg3 pgmodels.GetStatementsIntervalOptions) (pgmodels.StatementsInterval, bool, error) {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "GetStatementsInterval", arg0, arg1, arg2, arg3)
	ret0, _ := ret[0].(pgmodels.StatementsInterval)
	ret1, _ := ret[1].(bool)
	ret2, _ := ret[2].(error)
	return ret0, ret1, ret2
}

// GetStatementsInterval indicates an expected call of GetStatementsInterval.
func (mr *MockPerfDiagMockRecorder) GetStatementsInterval(arg0, arg1, arg2, arg3 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "GetStatementsInterval", reflect.TypeOf((*MockPerfDiag)(nil).GetStatementsInterval), arg0, arg1, arg2, arg3)
}

// GetStatementsStats mocks base method.
func (m *MockPerfDiag) GetStatementsStats(arg0 context.Context, arg1 string, arg2 int64, arg3 pgmodels.GetStatementsStatsOptions) (pgmodels.StatementsStats, bool, error) {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "GetStatementsStats", arg0, arg1, arg2, arg3)
	ret0, _ := ret[0].(pgmodels.StatementsStats)
	ret1, _ := ret[1].(bool)
	ret2, _ := ret[2].(error)
	return ret0, ret1, ret2
}

// GetStatementsStats indicates an expected call of GetStatementsStats.
func (mr *MockPerfDiagMockRecorder) GetStatementsStats(arg0, arg1, arg2, arg3 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "GetStatementsStats", reflect.TypeOf((*MockPerfDiag)(nil).GetStatementsStats), arg0, arg1, arg2, arg3)
}
