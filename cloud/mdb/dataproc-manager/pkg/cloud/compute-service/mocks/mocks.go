// Code generated by MockGen. DO NOT EDIT.
// Source: client.go

// Package mocks is a generated GoMock package.
package mocks

import (
	v1 "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/compute/v1"
	operation "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/operation"
	context "context"
	gomock "github.com/golang/mock/gomock"
	reflect "reflect"
)

// MockComputeServiceClient is a mock of ComputeServiceClient interface.
type MockComputeServiceClient struct {
	ctrl     *gomock.Controller
	recorder *MockComputeServiceClientMockRecorder
}

// MockComputeServiceClientMockRecorder is the mock recorder for MockComputeServiceClient.
type MockComputeServiceClientMockRecorder struct {
	mock *MockComputeServiceClient
}

// NewMockComputeServiceClient creates a new mock instance.
func NewMockComputeServiceClient(ctrl *gomock.Controller) *MockComputeServiceClient {
	mock := &MockComputeServiceClient{ctrl: ctrl}
	mock.recorder = &MockComputeServiceClientMockRecorder{mock}
	return mock
}

// EXPECT returns an object that allows the caller to indicate expected use.
func (m *MockComputeServiceClient) EXPECT() *MockComputeServiceClientMockRecorder {
	return m.recorder
}

// Get mocks base method.
func (m *MockComputeServiceClient) Get(ctx context.Context, instanceID string) (*v1.Instance, error) {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "Get", ctx, instanceID)
	ret0, _ := ret[0].(*v1.Instance)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// Get indicates an expected call of Get.
func (mr *MockComputeServiceClientMockRecorder) Get(ctx, instanceID interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "Get", reflect.TypeOf((*MockComputeServiceClient)(nil).Get), ctx, instanceID)
}

// Wait mocks base method.
func (m *MockComputeServiceClient) Wait(ctx context.Context, op *operation.Operation) error {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "Wait", ctx, op)
	ret0, _ := ret[0].(error)
	return ret0
}

// Wait indicates an expected call of Wait.
func (mr *MockComputeServiceClientMockRecorder) Wait(ctx, op interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "Wait", reflect.TypeOf((*MockComputeServiceClient)(nil).Wait), ctx, op)
}
