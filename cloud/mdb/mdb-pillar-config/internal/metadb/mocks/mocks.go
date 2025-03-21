// Code generated by MockGen. DO NOT EDIT.
// Source: a.yandex-team.ru/cloud/mdb/mdb-pillar-config/internal/metadb (interfaces: MetaDB)

// Package mocks is a generated GoMock package.
package mocks

import (
	optional "a.yandex-team.ru/cloud/mdb/internal/optional"
	metadb "a.yandex-team.ru/cloud/mdb/mdb-pillar-config/internal/metadb"
	context "context"
	json "encoding/json"
	gomock "github.com/golang/mock/gomock"
	hasql "golang.yandex/hasql"
	reflect "reflect"
)

// MockMetaDB is a mock of MetaDB interface.
type MockMetaDB struct {
	ctrl     *gomock.Controller
	recorder *MockMetaDBMockRecorder
}

// MockMetaDBMockRecorder is the mock recorder for MockMetaDB.
type MockMetaDBMockRecorder struct {
	mock *MockMetaDB
}

// NewMockMetaDB creates a new mock instance.
func NewMockMetaDB(ctrl *gomock.Controller) *MockMetaDB {
	mock := &MockMetaDB{ctrl: ctrl}
	mock.recorder = &MockMetaDBMockRecorder{mock}
	return mock
}

// EXPECT returns an object that allows the caller to indicate expected use.
func (m *MockMetaDB) EXPECT() *MockMetaDBMockRecorder {
	return m.recorder
}

// Begin mocks base method.
func (m *MockMetaDB) Begin(arg0 context.Context, arg1 hasql.NodeStateCriteria) (context.Context, error) {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "Begin", arg0, arg1)
	ret0, _ := ret[0].(context.Context)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// Begin indicates an expected call of Begin.
func (mr *MockMetaDBMockRecorder) Begin(arg0, arg1 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "Begin", reflect.TypeOf((*MockMetaDB)(nil).Begin), arg0, arg1)
}

// Close mocks base method.
func (m *MockMetaDB) Close() error {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "Close")
	ret0, _ := ret[0].(error)
	return ret0
}

// Close indicates an expected call of Close.
func (mr *MockMetaDBMockRecorder) Close() *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "Close", reflect.TypeOf((*MockMetaDB)(nil).Close))
}

// Commit mocks base method.
func (m *MockMetaDB) Commit(arg0 context.Context) error {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "Commit", arg0)
	ret0, _ := ret[0].(error)
	return ret0
}

// Commit indicates an expected call of Commit.
func (mr *MockMetaDBMockRecorder) Commit(arg0 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "Commit", reflect.TypeOf((*MockMetaDB)(nil).Commit), arg0)
}

// ConfigHostAuth mocks base method.
func (m *MockMetaDB) ConfigHostAuth(arg0 context.Context, arg1 string) (metadb.ConfigHostAuthInfo, error) {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "ConfigHostAuth", arg0, arg1)
	ret0, _ := ret[0].(metadb.ConfigHostAuthInfo)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// ConfigHostAuth indicates an expected call of ConfigHostAuth.
func (mr *MockMetaDBMockRecorder) ConfigHostAuth(arg0, arg1 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "ConfigHostAuth", reflect.TypeOf((*MockMetaDB)(nil).ConfigHostAuth), arg0, arg1)
}

// GenerateManagedConfig mocks base method.
func (m *MockMetaDB) GenerateManagedConfig(arg0 context.Context, arg1, arg2 string, arg3 optional.Int64) (json.RawMessage, error) {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "GenerateManagedConfig", arg0, arg1, arg2, arg3)
	ret0, _ := ret[0].(json.RawMessage)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// GenerateManagedConfig indicates an expected call of GenerateManagedConfig.
func (mr *MockMetaDBMockRecorder) GenerateManagedConfig(arg0, arg1, arg2, arg3 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "GenerateManagedConfig", reflect.TypeOf((*MockMetaDB)(nil).GenerateManagedConfig), arg0, arg1, arg2, arg3)
}

// GenerateUnmanagedConfig mocks base method.
func (m *MockMetaDB) GenerateUnmanagedConfig(arg0 context.Context, arg1, arg2 string, arg3 optional.Int64) (json.RawMessage, error) {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "GenerateUnmanagedConfig", arg0, arg1, arg2, arg3)
	ret0, _ := ret[0].(json.RawMessage)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// GenerateUnmanagedConfig indicates an expected call of GenerateUnmanagedConfig.
func (mr *MockMetaDBMockRecorder) GenerateUnmanagedConfig(arg0, arg1, arg2, arg3 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "GenerateUnmanagedConfig", reflect.TypeOf((*MockMetaDB)(nil).GenerateUnmanagedConfig), arg0, arg1, arg2, arg3)
}

// IsReady mocks base method.
func (m *MockMetaDB) IsReady(arg0 context.Context) error {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "IsReady", arg0)
	ret0, _ := ret[0].(error)
	return ret0
}

// IsReady indicates an expected call of IsReady.
func (mr *MockMetaDBMockRecorder) IsReady(arg0 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "IsReady", reflect.TypeOf((*MockMetaDB)(nil).IsReady), arg0)
}

// Rollback mocks base method.
func (m *MockMetaDB) Rollback(arg0 context.Context) error {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "Rollback", arg0)
	ret0, _ := ret[0].(error)
	return ret0
}

// Rollback indicates an expected call of Rollback.
func (mr *MockMetaDBMockRecorder) Rollback(arg0 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "Rollback", reflect.TypeOf((*MockMetaDB)(nil).Rollback), arg0)
}
