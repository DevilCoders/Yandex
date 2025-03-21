// Code generated by MockGen. DO NOT EDIT.
// Source: a.yandex-team.ru/cloud/mdb/backup/worker/internal/jobhandler (interfaces: JobHandler)

// Package mocks is a generated GoMock package.
package mocks

import (
	models "a.yandex-team.ru/cloud/mdb/backup/worker/internal/models"
	gomock "github.com/golang/mock/gomock"
	reflect "reflect"
)

// MockJobHandler is a mock of JobHandler interface.
type MockJobHandler struct {
	ctrl     *gomock.Controller
	recorder *MockJobHandlerMockRecorder
}

// MockJobHandlerMockRecorder is the mock recorder for MockJobHandler.
type MockJobHandlerMockRecorder struct {
	mock *MockJobHandler
}

// NewMockJobHandler creates a new mock instance.
func NewMockJobHandler(ctrl *gomock.Controller) *MockJobHandler {
	mock := &MockJobHandler{ctrl: ctrl}
	mock.recorder = &MockJobHandlerMockRecorder{mock}
	return mock
}

// EXPECT returns an object that allows the caller to indicate expected use.
func (m *MockJobHandler) EXPECT() *MockJobHandlerMockRecorder {
	return m.recorder
}

// HandleBackupJob mocks base method.
func (m *MockJobHandler) HandleBackupJob(arg0 models.BackupJob) error {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "HandleBackupJob", arg0)
	ret0, _ := ret[0].(error)
	return ret0
}

// HandleBackupJob indicates an expected call of HandleBackupJob.
func (mr *MockJobHandlerMockRecorder) HandleBackupJob(arg0 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "HandleBackupJob", reflect.TypeOf((*MockJobHandler)(nil).HandleBackupJob), arg0)
}
