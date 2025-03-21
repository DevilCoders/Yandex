// Code generated by MockGen. DO NOT EDIT.
// Source: a.yandex-team.ru/cloud/mdb/mdb-health/pkg/healthstore (interfaces: Backend)

// Package mocks is a generated GoMock package.
package mocks

import (
	healthstore "a.yandex-team.ru/cloud/mdb/mdb-health/pkg/healthstore"
	context "context"
	gomock "github.com/golang/mock/gomock"
	reflect "reflect"
)

// MockBackend is a mock of Backend interface.
type MockBackend struct {
	ctrl     *gomock.Controller
	recorder *MockBackendMockRecorder
}

// MockBackendMockRecorder is the mock recorder for MockBackend.
type MockBackendMockRecorder struct {
	mock *MockBackend
}

// NewMockBackend creates a new mock instance.
func NewMockBackend(ctrl *gomock.Controller) *MockBackend {
	mock := &MockBackend{ctrl: ctrl}
	mock.recorder = &MockBackendMockRecorder{mock}
	return mock
}

// EXPECT returns an object that allows the caller to indicate expected use.
func (m *MockBackend) EXPECT() *MockBackendMockRecorder {
	return m.recorder
}

// Close mocks base method.
func (m *MockBackend) Close() error {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "Close")
	ret0, _ := ret[0].(error)
	return ret0
}

// Close indicates an expected call of Close.
func (mr *MockBackendMockRecorder) Close() *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "Close", reflect.TypeOf((*MockBackend)(nil).Close))
}

// IsReady mocks base method.
func (m *MockBackend) IsReady(arg0 context.Context) error {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "IsReady", arg0)
	ret0, _ := ret[0].(error)
	return ret0
}

// IsReady indicates an expected call of IsReady.
func (mr *MockBackendMockRecorder) IsReady(arg0 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "IsReady", reflect.TypeOf((*MockBackend)(nil).IsReady), arg0)
}

// Name mocks base method.
func (m *MockBackend) Name() string {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "Name")
	ret0, _ := ret[0].(string)
	return ret0
}

// Name indicates an expected call of Name.
func (mr *MockBackendMockRecorder) Name() *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "Name", reflect.TypeOf((*MockBackend)(nil).Name))
}

// StoreHostsHealth mocks base method.
func (m *MockBackend) StoreHostsHealth(arg0 context.Context, arg1 []healthstore.HostHealthToStore) error {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "StoreHostsHealth", arg0, arg1)
	ret0, _ := ret[0].(error)
	return ret0
}

// StoreHostsHealth indicates an expected call of StoreHostsHealth.
func (mr *MockBackendMockRecorder) StoreHostsHealth(arg0, arg1 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "StoreHostsHealth", reflect.TypeOf((*MockBackend)(nil).StoreHostsHealth), arg0, arg1)
}
