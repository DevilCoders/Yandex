// Code generated by MockGen. DO NOT EDIT.
// Source: a.yandex-team.ru/cloud/mdb/internal/compute/vpc (interfaces: Client)

// Package mocks is a generated GoMock package.
package mocks

import (
	vpc "a.yandex-team.ru/cloud/mdb/internal/compute/vpc"
	network "a.yandex-team.ru/cloud/mdb/internal/network"
	context "context"
	gomock "github.com/golang/mock/gomock"
	reflect "reflect"
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

// GetNetwork mocks base method.
func (m *MockClient) GetNetwork(arg0 context.Context, arg1 string) (network.Network, error) {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "GetNetwork", arg0, arg1)
	ret0, _ := ret[0].(network.Network)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// GetNetwork indicates an expected call of GetNetwork.
func (mr *MockClientMockRecorder) GetNetwork(arg0, arg1 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "GetNetwork", reflect.TypeOf((*MockClient)(nil).GetNetwork), arg0, arg1)
}

// GetSecurityGroup mocks base method.
func (m *MockClient) GetSecurityGroup(arg0 context.Context, arg1 string) (vpc.SecurityGroup, error) {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "GetSecurityGroup", arg0, arg1)
	ret0, _ := ret[0].(vpc.SecurityGroup)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// GetSecurityGroup indicates an expected call of GetSecurityGroup.
func (mr *MockClientMockRecorder) GetSecurityGroup(arg0, arg1 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "GetSecurityGroup", reflect.TypeOf((*MockClient)(nil).GetSecurityGroup), arg0, arg1)
}

// GetSubnet mocks base method.
func (m *MockClient) GetSubnet(arg0 context.Context, arg1 string) (network.Subnet, error) {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "GetSubnet", arg0, arg1)
	ret0, _ := ret[0].(network.Subnet)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// GetSubnet indicates an expected call of GetSubnet.
func (mr *MockClientMockRecorder) GetSubnet(arg0, arg1 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "GetSubnet", reflect.TypeOf((*MockClient)(nil).GetSubnet), arg0, arg1)
}

// GetSubnets mocks base method.
func (m *MockClient) GetSubnets(arg0 context.Context, arg1 network.Network) ([]network.Subnet, error) {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "GetSubnets", arg0, arg1)
	ret0, _ := ret[0].([]network.Subnet)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// GetSubnets indicates an expected call of GetSubnets.
func (mr *MockClientMockRecorder) GetSubnets(arg0, arg1 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "GetSubnets", reflect.TypeOf((*MockClient)(nil).GetSubnets), arg0, arg1)
}
