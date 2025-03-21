// Code generated by MockGen. DO NOT EDIT.
// Source: a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/helpers/juggler (interfaces: JugglerChecker)

// Package mocks is a generated GoMock package.
package mocks

import (
	juggler "a.yandex-team.ru/cloud/mdb/cms/api/internal/logic/autoduty/helpers/juggler"
	context "context"
	gomock "github.com/golang/mock/gomock"
	reflect "reflect"
	time "time"
)

// MockJugglerChecker is a mock of JugglerChecker interface.
type MockJugglerChecker struct {
	ctrl     *gomock.Controller
	recorder *MockJugglerCheckerMockRecorder
}

// MockJugglerCheckerMockRecorder is the mock recorder for MockJugglerChecker.
type MockJugglerCheckerMockRecorder struct {
	mock *MockJugglerChecker
}

// NewMockJugglerChecker creates a new mock instance.
func NewMockJugglerChecker(ctrl *gomock.Controller) *MockJugglerChecker {
	mock := &MockJugglerChecker{ctrl: ctrl}
	mock.recorder = &MockJugglerCheckerMockRecorder{mock}
	return mock
}

// EXPECT returns an object that allows the caller to indicate expected use.
func (m *MockJugglerChecker) EXPECT() *MockJugglerCheckerMockRecorder {
	return m.recorder
}

// Check mocks base method.
func (m *MockJugglerChecker) Check(arg0 context.Context, arg1, arg2 []string, arg3 time.Time) (juggler.FQDNGroupByJugglerCheck, error) {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "Check", arg0, arg1, arg2, arg3)
	ret0, _ := ret[0].(juggler.FQDNGroupByJugglerCheck)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// Check indicates an expected call of Check.
func (mr *MockJugglerCheckerMockRecorder) Check(arg0, arg1, arg2, arg3 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "Check", reflect.TypeOf((*MockJugglerChecker)(nil).Check), arg0, arg1, arg2, arg3)
}
