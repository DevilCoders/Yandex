// Code generated by MockGen. DO NOT EDIT.
// Source: a.yandex-team.ru/cloud/mdb/mlock/pkg/mlockclient (interfaces: Locker)

// Package mocks is a generated GoMock package.
package mocks

import (
	mlockclient "a.yandex-team.ru/cloud/mdb/mlock/pkg/mlockclient"
	context "context"
	gomock "github.com/golang/mock/gomock"
	reflect "reflect"
)

// MockLocker is a mock of Locker interface.
type MockLocker struct {
	ctrl     *gomock.Controller
	recorder *MockLockerMockRecorder
}

// MockLockerMockRecorder is the mock recorder for MockLocker.
type MockLockerMockRecorder struct {
	mock *MockLocker
}

// NewMockLocker creates a new mock instance.
func NewMockLocker(ctrl *gomock.Controller) *MockLocker {
	mock := &MockLocker{ctrl: ctrl}
	mock.recorder = &MockLockerMockRecorder{mock}
	return mock
}

// EXPECT returns an object that allows the caller to indicate expected use.
func (m *MockLocker) EXPECT() *MockLockerMockRecorder {
	return m.recorder
}

// CreateLock mocks base method.
func (m *MockLocker) CreateLock(arg0 context.Context, arg1, arg2 string, arg3 []string, arg4 string) error {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "CreateLock", arg0, arg1, arg2, arg3, arg4)
	ret0, _ := ret[0].(error)
	return ret0
}

// CreateLock indicates an expected call of CreateLock.
func (mr *MockLockerMockRecorder) CreateLock(arg0, arg1, arg2, arg3, arg4 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "CreateLock", reflect.TypeOf((*MockLocker)(nil).CreateLock), arg0, arg1, arg2, arg3, arg4)
}

// GetLockStatus mocks base method.
func (m *MockLocker) GetLockStatus(arg0 context.Context, arg1 string) (mlockclient.LockStatus, error) {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "GetLockStatus", arg0, arg1)
	ret0, _ := ret[0].(mlockclient.LockStatus)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// GetLockStatus indicates an expected call of GetLockStatus.
func (mr *MockLockerMockRecorder) GetLockStatus(arg0, arg1 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "GetLockStatus", reflect.TypeOf((*MockLocker)(nil).GetLockStatus), arg0, arg1)
}

// ReleaseLock mocks base method.
func (m *MockLocker) ReleaseLock(arg0 context.Context, arg1 string) error {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "ReleaseLock", arg0, arg1)
	ret0, _ := ret[0].(error)
	return ret0
}

// ReleaseLock indicates an expected call of ReleaseLock.
func (mr *MockLockerMockRecorder) ReleaseLock(arg0, arg1 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "ReleaseLock", reflect.TypeOf((*MockLocker)(nil).ReleaseLock), arg0, arg1)
}
