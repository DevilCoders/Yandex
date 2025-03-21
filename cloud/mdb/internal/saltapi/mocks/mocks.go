// Code generated by MockGen. DO NOT EDIT.
// Source: a.yandex-team.ru/cloud/mdb/internal/saltapi (interfaces: Auth,Authenticator,Secrets,Client,SaltUtil,State,Test,Config)

// Package mocks is a generated GoMock package.
package mocks

import (
	saltapi "a.yandex-team.ru/cloud/mdb/internal/saltapi"
	log "a.yandex-team.ru/library/go/core/log"
	context "context"
	gomock "github.com/golang/mock/gomock"
	reflect "reflect"
	time "time"
)

// MockAuth is a mock of Auth interface.
type MockAuth struct {
	ctrl     *gomock.Controller
	recorder *MockAuthMockRecorder
}

// MockAuthMockRecorder is the mock recorder for MockAuth.
type MockAuthMockRecorder struct {
	mock *MockAuth
}

// NewMockAuth creates a new mock instance.
func NewMockAuth(ctrl *gomock.Controller) *MockAuth {
	mock := &MockAuth{ctrl: ctrl}
	mock.recorder = &MockAuthMockRecorder{mock}
	return mock
}

// EXPECT returns an object that allows the caller to indicate expected use.
func (m *MockAuth) EXPECT() *MockAuthMockRecorder {
	return m.recorder
}

// AuthEAuthToken mocks base method.
func (m *MockAuth) AuthEAuthToken(arg0 context.Context, arg1 time.Duration) (saltapi.Token, error) {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "AuthEAuthToken", arg0, arg1)
	ret0, _ := ret[0].(saltapi.Token)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// AuthEAuthToken indicates an expected call of AuthEAuthToken.
func (mr *MockAuthMockRecorder) AuthEAuthToken(arg0, arg1 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "AuthEAuthToken", reflect.TypeOf((*MockAuth)(nil).AuthEAuthToken), arg0, arg1)
}

// AuthSessionToken mocks base method.
func (m *MockAuth) AuthSessionToken(arg0 context.Context) (saltapi.Token, error) {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "AuthSessionToken", arg0)
	ret0, _ := ret[0].(saltapi.Token)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// AuthSessionToken indicates an expected call of AuthSessionToken.
func (mr *MockAuthMockRecorder) AuthSessionToken(arg0 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "AuthSessionToken", reflect.TypeOf((*MockAuth)(nil).AuthSessionToken), arg0)
}

// AutoAuth mocks base method.
func (m *MockAuth) AutoAuth(arg0 context.Context, arg1, arg2 time.Duration, arg3 log.Logger) {
	m.ctrl.T.Helper()
	m.ctrl.Call(m, "AutoAuth", arg0, arg1, arg2, arg3)
}

// AutoAuth indicates an expected call of AutoAuth.
func (mr *MockAuthMockRecorder) AutoAuth(arg0, arg1, arg2, arg3 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "AutoAuth", reflect.TypeOf((*MockAuth)(nil).AutoAuth), arg0, arg1, arg2, arg3)
}

// Credentials mocks base method.
func (m *MockAuth) Credentials() saltapi.Credentials {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "Credentials")
	ret0, _ := ret[0].(saltapi.Credentials)
	return ret0
}

// Credentials indicates an expected call of Credentials.
func (mr *MockAuthMockRecorder) Credentials() *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "Credentials", reflect.TypeOf((*MockAuth)(nil).Credentials))
}

// Invalidate mocks base method.
func (m *MockAuth) Invalidate() {
	m.ctrl.T.Helper()
	m.ctrl.Call(m, "Invalidate")
}

// Invalidate indicates an expected call of Invalidate.
func (mr *MockAuthMockRecorder) Invalidate() *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "Invalidate", reflect.TypeOf((*MockAuth)(nil).Invalidate))
}

// Tokens mocks base method.
func (m *MockAuth) Tokens() saltapi.Tokens {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "Tokens")
	ret0, _ := ret[0].(saltapi.Tokens)
	return ret0
}

// Tokens indicates an expected call of Tokens.
func (mr *MockAuthMockRecorder) Tokens() *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "Tokens", reflect.TypeOf((*MockAuth)(nil).Tokens))
}

// MockAuthenticator is a mock of Authenticator interface.
type MockAuthenticator struct {
	ctrl     *gomock.Controller
	recorder *MockAuthenticatorMockRecorder
}

// MockAuthenticatorMockRecorder is the mock recorder for MockAuthenticator.
type MockAuthenticatorMockRecorder struct {
	mock *MockAuthenticator
}

// NewMockAuthenticator creates a new mock instance.
func NewMockAuthenticator(ctrl *gomock.Controller) *MockAuthenticator {
	mock := &MockAuthenticator{ctrl: ctrl}
	mock.recorder = &MockAuthenticatorMockRecorder{mock}
	return mock
}

// EXPECT returns an object that allows the caller to indicate expected use.
func (m *MockAuthenticator) EXPECT() *MockAuthenticatorMockRecorder {
	return m.recorder
}

// AuthEAuthToken mocks base method.
func (m *MockAuthenticator) AuthEAuthToken(arg0 context.Context, arg1 time.Duration) (saltapi.Token, error) {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "AuthEAuthToken", arg0, arg1)
	ret0, _ := ret[0].(saltapi.Token)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// AuthEAuthToken indicates an expected call of AuthEAuthToken.
func (mr *MockAuthenticatorMockRecorder) AuthEAuthToken(arg0, arg1 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "AuthEAuthToken", reflect.TypeOf((*MockAuthenticator)(nil).AuthEAuthToken), arg0, arg1)
}

// AuthSessionToken mocks base method.
func (m *MockAuthenticator) AuthSessionToken(arg0 context.Context) (saltapi.Token, error) {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "AuthSessionToken", arg0)
	ret0, _ := ret[0].(saltapi.Token)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// AuthSessionToken indicates an expected call of AuthSessionToken.
func (mr *MockAuthenticatorMockRecorder) AuthSessionToken(arg0 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "AuthSessionToken", reflect.TypeOf((*MockAuthenticator)(nil).AuthSessionToken), arg0)
}

// AutoAuth mocks base method.
func (m *MockAuthenticator) AutoAuth(arg0 context.Context, arg1, arg2 time.Duration, arg3 log.Logger) {
	m.ctrl.T.Helper()
	m.ctrl.Call(m, "AutoAuth", arg0, arg1, arg2, arg3)
}

// AutoAuth indicates an expected call of AutoAuth.
func (mr *MockAuthenticatorMockRecorder) AutoAuth(arg0, arg1, arg2, arg3 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "AutoAuth", reflect.TypeOf((*MockAuthenticator)(nil).AutoAuth), arg0, arg1, arg2, arg3)
}

// MockSecrets is a mock of Secrets interface.
type MockSecrets struct {
	ctrl     *gomock.Controller
	recorder *MockSecretsMockRecorder
}

// MockSecretsMockRecorder is the mock recorder for MockSecrets.
type MockSecretsMockRecorder struct {
	mock *MockSecrets
}

// NewMockSecrets creates a new mock instance.
func NewMockSecrets(ctrl *gomock.Controller) *MockSecrets {
	mock := &MockSecrets{ctrl: ctrl}
	mock.recorder = &MockSecretsMockRecorder{mock}
	return mock
}

// EXPECT returns an object that allows the caller to indicate expected use.
func (m *MockSecrets) EXPECT() *MockSecretsMockRecorder {
	return m.recorder
}

// Credentials mocks base method.
func (m *MockSecrets) Credentials() saltapi.Credentials {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "Credentials")
	ret0, _ := ret[0].(saltapi.Credentials)
	return ret0
}

// Credentials indicates an expected call of Credentials.
func (mr *MockSecretsMockRecorder) Credentials() *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "Credentials", reflect.TypeOf((*MockSecrets)(nil).Credentials))
}

// Invalidate mocks base method.
func (m *MockSecrets) Invalidate() {
	m.ctrl.T.Helper()
	m.ctrl.Call(m, "Invalidate")
}

// Invalidate indicates an expected call of Invalidate.
func (mr *MockSecretsMockRecorder) Invalidate() *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "Invalidate", reflect.TypeOf((*MockSecrets)(nil).Invalidate))
}

// Tokens mocks base method.
func (m *MockSecrets) Tokens() saltapi.Tokens {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "Tokens")
	ret0, _ := ret[0].(saltapi.Tokens)
	return ret0
}

// Tokens indicates an expected call of Tokens.
func (mr *MockSecretsMockRecorder) Tokens() *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "Tokens", reflect.TypeOf((*MockSecrets)(nil).Tokens))
}

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

// AsyncRun mocks base method.
func (m *MockClient) AsyncRun(arg0 context.Context, arg1 saltapi.Secrets, arg2 time.Duration, arg3, arg4 string, arg5 ...string) (string, error) {
	m.ctrl.T.Helper()
	varargs := []interface{}{arg0, arg1, arg2, arg3, arg4}
	for _, a := range arg5 {
		varargs = append(varargs, a)
	}
	ret := m.ctrl.Call(m, "AsyncRun", varargs...)
	ret0, _ := ret[0].(string)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// AsyncRun indicates an expected call of AsyncRun.
func (mr *MockClientMockRecorder) AsyncRun(arg0, arg1, arg2, arg3, arg4 interface{}, arg5 ...interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	varargs := append([]interface{}{arg0, arg1, arg2, arg3, arg4}, arg5...)
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "AsyncRun", reflect.TypeOf((*MockClient)(nil).AsyncRun), varargs...)
}

// Config mocks base method.
func (m *MockClient) Config() saltapi.Config {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "Config")
	ret0, _ := ret[0].(saltapi.Config)
	return ret0
}

// Config indicates an expected call of Config.
func (mr *MockClientMockRecorder) Config() *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "Config", reflect.TypeOf((*MockClient)(nil).Config))
}

// Minions mocks base method.
func (m *MockClient) Minions(arg0 context.Context, arg1 saltapi.Secrets) ([]saltapi.Minion, error) {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "Minions", arg0, arg1)
	ret0, _ := ret[0].([]saltapi.Minion)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// Minions indicates an expected call of Minions.
func (mr *MockClientMockRecorder) Minions(arg0, arg1 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "Minions", reflect.TypeOf((*MockClient)(nil).Minions), arg0, arg1)
}

// NewAuth mocks base method.
func (m *MockClient) NewAuth(arg0 saltapi.Credentials) saltapi.Auth {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "NewAuth", arg0)
	ret0, _ := ret[0].(saltapi.Auth)
	return ret0
}

// NewAuth indicates an expected call of NewAuth.
func (mr *MockClientMockRecorder) NewAuth(arg0 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "NewAuth", reflect.TypeOf((*MockClient)(nil).NewAuth), arg0)
}

// Ping mocks base method.
func (m *MockClient) Ping(arg0 context.Context, arg1 saltapi.Secrets) error {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "Ping", arg0, arg1)
	ret0, _ := ret[0].(error)
	return ret0
}

// Ping indicates an expected call of Ping.
func (mr *MockClientMockRecorder) Ping(arg0, arg1 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "Ping", reflect.TypeOf((*MockClient)(nil).Ping), arg0, arg1)
}

// Run mocks base method.
func (m *MockClient) Run(arg0 context.Context, arg1 saltapi.Secrets, arg2 time.Duration, arg3, arg4 string, arg5 ...string) error {
	m.ctrl.T.Helper()
	varargs := []interface{}{arg0, arg1, arg2, arg3, arg4}
	for _, a := range arg5 {
		varargs = append(varargs, a)
	}
	ret := m.ctrl.Call(m, "Run", varargs...)
	ret0, _ := ret[0].(error)
	return ret0
}

// Run indicates an expected call of Run.
func (mr *MockClientMockRecorder) Run(arg0, arg1, arg2, arg3, arg4 interface{}, arg5 ...interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	varargs := append([]interface{}{arg0, arg1, arg2, arg3, arg4}, arg5...)
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "Run", reflect.TypeOf((*MockClient)(nil).Run), varargs...)
}

// SaltUtil mocks base method.
func (m *MockClient) SaltUtil() saltapi.SaltUtil {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "SaltUtil")
	ret0, _ := ret[0].(saltapi.SaltUtil)
	return ret0
}

// SaltUtil indicates an expected call of SaltUtil.
func (mr *MockClientMockRecorder) SaltUtil() *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "SaltUtil", reflect.TypeOf((*MockClient)(nil).SaltUtil))
}

// State mocks base method.
func (m *MockClient) State() saltapi.State {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "State")
	ret0, _ := ret[0].(saltapi.State)
	return ret0
}

// State indicates an expected call of State.
func (mr *MockClientMockRecorder) State() *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "State", reflect.TypeOf((*MockClient)(nil).State))
}

// Test mocks base method.
func (m *MockClient) Test() saltapi.Test {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "Test")
	ret0, _ := ret[0].(saltapi.Test)
	return ret0
}

// Test indicates an expected call of Test.
func (mr *MockClientMockRecorder) Test() *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "Test", reflect.TypeOf((*MockClient)(nil).Test))
}

// MockSaltUtil is a mock of SaltUtil interface.
type MockSaltUtil struct {
	ctrl     *gomock.Controller
	recorder *MockSaltUtilMockRecorder
}

// MockSaltUtilMockRecorder is the mock recorder for MockSaltUtil.
type MockSaltUtilMockRecorder struct {
	mock *MockSaltUtil
}

// NewMockSaltUtil creates a new mock instance.
func NewMockSaltUtil(ctrl *gomock.Controller) *MockSaltUtil {
	mock := &MockSaltUtil{ctrl: ctrl}
	mock.recorder = &MockSaltUtilMockRecorder{mock}
	return mock
}

// EXPECT returns an object that allows the caller to indicate expected use.
func (m *MockSaltUtil) EXPECT() *MockSaltUtilMockRecorder {
	return m.recorder
}

// AsyncFindJob mocks base method.
func (m *MockSaltUtil) AsyncFindJob(arg0 context.Context, arg1 saltapi.Secrets, arg2 time.Duration, arg3, arg4 string) (string, error) {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "AsyncFindJob", arg0, arg1, arg2, arg3, arg4)
	ret0, _ := ret[0].(string)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// AsyncFindJob indicates an expected call of AsyncFindJob.
func (mr *MockSaltUtilMockRecorder) AsyncFindJob(arg0, arg1, arg2, arg3, arg4 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "AsyncFindJob", reflect.TypeOf((*MockSaltUtil)(nil).AsyncFindJob), arg0, arg1, arg2, arg3, arg4)
}

// AsyncIsRunning mocks base method.
func (m *MockSaltUtil) AsyncIsRunning(arg0 context.Context, arg1 saltapi.Secrets, arg2 time.Duration, arg3, arg4 string) (string, error) {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "AsyncIsRunning", arg0, arg1, arg2, arg3, arg4)
	ret0, _ := ret[0].(string)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// AsyncIsRunning indicates an expected call of AsyncIsRunning.
func (mr *MockSaltUtilMockRecorder) AsyncIsRunning(arg0, arg1, arg2, arg3, arg4 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "AsyncIsRunning", reflect.TypeOf((*MockSaltUtil)(nil).AsyncIsRunning), arg0, arg1, arg2, arg3, arg4)
}

// AsyncKillJob mocks base method.
func (m *MockSaltUtil) AsyncKillJob(arg0 context.Context, arg1 saltapi.Secrets, arg2 time.Duration, arg3, arg4 string) (string, error) {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "AsyncKillJob", arg0, arg1, arg2, arg3, arg4)
	ret0, _ := ret[0].(string)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// AsyncKillJob indicates an expected call of AsyncKillJob.
func (mr *MockSaltUtilMockRecorder) AsyncKillJob(arg0, arg1, arg2, arg3, arg4 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "AsyncKillJob", reflect.TypeOf((*MockSaltUtil)(nil).AsyncKillJob), arg0, arg1, arg2, arg3, arg4)
}

// AsyncSyncAll mocks base method.
func (m *MockSaltUtil) AsyncSyncAll(arg0 context.Context, arg1 saltapi.Secrets, arg2 time.Duration, arg3 string) (string, error) {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "AsyncSyncAll", arg0, arg1, arg2, arg3)
	ret0, _ := ret[0].(string)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// AsyncSyncAll indicates an expected call of AsyncSyncAll.
func (mr *MockSaltUtilMockRecorder) AsyncSyncAll(arg0, arg1, arg2, arg3 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "AsyncSyncAll", reflect.TypeOf((*MockSaltUtil)(nil).AsyncSyncAll), arg0, arg1, arg2, arg3)
}

// AsyncTermJob mocks base method.
func (m *MockSaltUtil) AsyncTermJob(arg0 context.Context, arg1 saltapi.Secrets, arg2 time.Duration, arg3, arg4 string) (string, error) {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "AsyncTermJob", arg0, arg1, arg2, arg3, arg4)
	ret0, _ := ret[0].(string)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// AsyncTermJob indicates an expected call of AsyncTermJob.
func (mr *MockSaltUtilMockRecorder) AsyncTermJob(arg0, arg1, arg2, arg3, arg4 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "AsyncTermJob", reflect.TypeOf((*MockSaltUtil)(nil).AsyncTermJob), arg0, arg1, arg2, arg3, arg4)
}

// FindJob mocks base method.
func (m *MockSaltUtil) FindJob(arg0 context.Context, arg1 saltapi.Secrets, arg2 time.Duration, arg3, arg4 string) (map[string]saltapi.RunningFunc, error) {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "FindJob", arg0, arg1, arg2, arg3, arg4)
	ret0, _ := ret[0].(map[string]saltapi.RunningFunc)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// FindJob indicates an expected call of FindJob.
func (mr *MockSaltUtilMockRecorder) FindJob(arg0, arg1, arg2, arg3, arg4 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "FindJob", reflect.TypeOf((*MockSaltUtil)(nil).FindJob), arg0, arg1, arg2, arg3, arg4)
}

// IsRunning mocks base method.
func (m *MockSaltUtil) IsRunning(arg0 context.Context, arg1 saltapi.Secrets, arg2 time.Duration, arg3, arg4 string) (map[string][]saltapi.RunningFunc, error) {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "IsRunning", arg0, arg1, arg2, arg3, arg4)
	ret0, _ := ret[0].(map[string][]saltapi.RunningFunc)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// IsRunning indicates an expected call of IsRunning.
func (mr *MockSaltUtilMockRecorder) IsRunning(arg0, arg1, arg2, arg3, arg4 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "IsRunning", reflect.TypeOf((*MockSaltUtil)(nil).IsRunning), arg0, arg1, arg2, arg3, arg4)
}

// KillJob mocks base method.
func (m *MockSaltUtil) KillJob(arg0 context.Context, arg1 saltapi.Secrets, arg2 time.Duration, arg3, arg4 string) error {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "KillJob", arg0, arg1, arg2, arg3, arg4)
	ret0, _ := ret[0].(error)
	return ret0
}

// KillJob indicates an expected call of KillJob.
func (mr *MockSaltUtilMockRecorder) KillJob(arg0, arg1, arg2, arg3, arg4 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "KillJob", reflect.TypeOf((*MockSaltUtil)(nil).KillJob), arg0, arg1, arg2, arg3, arg4)
}

// SyncAll mocks base method.
func (m *MockSaltUtil) SyncAll(arg0 context.Context, arg1 saltapi.Secrets, arg2 time.Duration, arg3 string) error {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "SyncAll", arg0, arg1, arg2, arg3)
	ret0, _ := ret[0].(error)
	return ret0
}

// SyncAll indicates an expected call of SyncAll.
func (mr *MockSaltUtilMockRecorder) SyncAll(arg0, arg1, arg2, arg3 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "SyncAll", reflect.TypeOf((*MockSaltUtil)(nil).SyncAll), arg0, arg1, arg2, arg3)
}

// TermJob mocks base method.
func (m *MockSaltUtil) TermJob(arg0 context.Context, arg1 saltapi.Secrets, arg2 time.Duration, arg3, arg4 string) error {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "TermJob", arg0, arg1, arg2, arg3, arg4)
	ret0, _ := ret[0].(error)
	return ret0
}

// TermJob indicates an expected call of TermJob.
func (mr *MockSaltUtilMockRecorder) TermJob(arg0, arg1, arg2, arg3, arg4 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "TermJob", reflect.TypeOf((*MockSaltUtil)(nil).TermJob), arg0, arg1, arg2, arg3, arg4)
}

// MockState is a mock of State interface.
type MockState struct {
	ctrl     *gomock.Controller
	recorder *MockStateMockRecorder
}

// MockStateMockRecorder is the mock recorder for MockState.
type MockStateMockRecorder struct {
	mock *MockState
}

// NewMockState creates a new mock instance.
func NewMockState(ctrl *gomock.Controller) *MockState {
	mock := &MockState{ctrl: ctrl}
	mock.recorder = &MockStateMockRecorder{mock}
	return mock
}

// EXPECT returns an object that allows the caller to indicate expected use.
func (m *MockState) EXPECT() *MockStateMockRecorder {
	return m.recorder
}

// AsyncHighstate mocks base method.
func (m *MockState) AsyncHighstate(arg0 context.Context, arg1 saltapi.Secrets, arg2 time.Duration, arg3 string, arg4 ...string) (string, error) {
	m.ctrl.T.Helper()
	varargs := []interface{}{arg0, arg1, arg2, arg3}
	for _, a := range arg4 {
		varargs = append(varargs, a)
	}
	ret := m.ctrl.Call(m, "AsyncHighstate", varargs...)
	ret0, _ := ret[0].(string)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// AsyncHighstate indicates an expected call of AsyncHighstate.
func (mr *MockStateMockRecorder) AsyncHighstate(arg0, arg1, arg2, arg3 interface{}, arg4 ...interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	varargs := append([]interface{}{arg0, arg1, arg2, arg3}, arg4...)
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "AsyncHighstate", reflect.TypeOf((*MockState)(nil).AsyncHighstate), varargs...)
}

// Highstate mocks base method.
func (m *MockState) Highstate(arg0 context.Context, arg1 saltapi.Secrets, arg2 time.Duration, arg3 string, arg4 ...string) error {
	m.ctrl.T.Helper()
	varargs := []interface{}{arg0, arg1, arg2, arg3}
	for _, a := range arg4 {
		varargs = append(varargs, a)
	}
	ret := m.ctrl.Call(m, "Highstate", varargs...)
	ret0, _ := ret[0].(error)
	return ret0
}

// Highstate indicates an expected call of Highstate.
func (mr *MockStateMockRecorder) Highstate(arg0, arg1, arg2, arg3 interface{}, arg4 ...interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	varargs := append([]interface{}{arg0, arg1, arg2, arg3}, arg4...)
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "Highstate", reflect.TypeOf((*MockState)(nil).Highstate), varargs...)
}

// Running mocks base method.
func (m *MockState) Running(arg0 context.Context, arg1 saltapi.Secrets, arg2 time.Duration, arg3 string) (map[string][]string, error) {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "Running", arg0, arg1, arg2, arg3)
	ret0, _ := ret[0].(map[string][]string)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// Running indicates an expected call of Running.
func (mr *MockStateMockRecorder) Running(arg0, arg1, arg2, arg3 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "Running", reflect.TypeOf((*MockState)(nil).Running), arg0, arg1, arg2, arg3)
}

// MockTest is a mock of Test interface.
type MockTest struct {
	ctrl     *gomock.Controller
	recorder *MockTestMockRecorder
}

// MockTestMockRecorder is the mock recorder for MockTest.
type MockTestMockRecorder struct {
	mock *MockTest
}

// NewMockTest creates a new mock instance.
func NewMockTest(ctrl *gomock.Controller) *MockTest {
	mock := &MockTest{ctrl: ctrl}
	mock.recorder = &MockTestMockRecorder{mock}
	return mock
}

// EXPECT returns an object that allows the caller to indicate expected use.
func (m *MockTest) EXPECT() *MockTestMockRecorder {
	return m.recorder
}

// AsyncPing mocks base method.
func (m *MockTest) AsyncPing(arg0 context.Context, arg1 saltapi.Secrets, arg2 time.Duration, arg3 string) (string, error) {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "AsyncPing", arg0, arg1, arg2, arg3)
	ret0, _ := ret[0].(string)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// AsyncPing indicates an expected call of AsyncPing.
func (mr *MockTestMockRecorder) AsyncPing(arg0, arg1, arg2, arg3 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "AsyncPing", reflect.TypeOf((*MockTest)(nil).AsyncPing), arg0, arg1, arg2, arg3)
}

// Ping mocks base method.
func (m *MockTest) Ping(arg0 context.Context, arg1 saltapi.Secrets, arg2 time.Duration, arg3 string) (map[string]bool, error) {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "Ping", arg0, arg1, arg2, arg3)
	ret0, _ := ret[0].(map[string]bool)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// Ping indicates an expected call of Ping.
func (mr *MockTestMockRecorder) Ping(arg0, arg1, arg2, arg3 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "Ping", reflect.TypeOf((*MockTest)(nil).Ping), arg0, arg1, arg2, arg3)
}

// MockConfig is a mock of Config interface.
type MockConfig struct {
	ctrl     *gomock.Controller
	recorder *MockConfigMockRecorder
}

// MockConfigMockRecorder is the mock recorder for MockConfig.
type MockConfigMockRecorder struct {
	mock *MockConfig
}

// NewMockConfig creates a new mock instance.
func NewMockConfig(ctrl *gomock.Controller) *MockConfig {
	mock := &MockConfig{ctrl: ctrl}
	mock.recorder = &MockConfigMockRecorder{mock}
	return mock
}

// EXPECT returns an object that allows the caller to indicate expected use.
func (m *MockConfig) EXPECT() *MockConfigMockRecorder {
	return m.recorder
}

// WorkerThreads mocks base method.
func (m *MockConfig) WorkerThreads(arg0 context.Context, arg1 saltapi.Secrets, arg2 time.Duration) (int32, error) {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "WorkerThreads", arg0, arg1, arg2)
	ret0, _ := ret[0].(int32)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// WorkerThreads indicates an expected call of WorkerThreads.
func (mr *MockConfigMockRecorder) WorkerThreads(arg0, arg1, arg2 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "WorkerThreads", reflect.TypeOf((*MockConfig)(nil).WorkerThreads), arg0, arg1, arg2)
}
