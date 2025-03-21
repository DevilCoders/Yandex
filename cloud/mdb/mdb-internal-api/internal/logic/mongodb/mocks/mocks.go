// Code generated by MockGen. DO NOT EDIT.
// Source: a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/mongodb (interfaces: MongoDB)

// Package mocks is a generated GoMock package.
package mocks

import (
	context "context"
	reflect "reflect"

	mongodb "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/mongodb"
	mongomodels "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/mongodb/mongomodels"
	clusters "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	operations "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/operations"
	gomock "github.com/golang/mock/gomock"
)

// MockMongoDB is a mock of MongoDB interface.
type MockMongoDB struct {
	ctrl     *gomock.Controller
	recorder *MockMongoDBMockRecorder
}

// MockMongoDBMockRecorder is the mock recorder for MockMongoDB.
type MockMongoDBMockRecorder struct {
	mock *MockMongoDB
}

// NewMockMongoDB creates a new mock instance.
func NewMockMongoDB(ctrl *gomock.Controller) *MockMongoDB {
	mock := &MockMongoDB{ctrl: ctrl}
	mock.recorder = &MockMongoDBMockRecorder{mock}
	return mock
}

// EXPECT returns an object that allows the caller to indicate expected use.
func (m *MockMongoDB) EXPECT() *MockMongoDBMockRecorder {
	return m.recorder
}

// Cluster mocks base method.
func (m *MockMongoDB) Cluster(arg0 context.Context, arg1 string) (clusters.ClusterExtended, error) {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "Cluster", arg0, arg1)
	ret0, _ := ret[0].(clusters.ClusterExtended)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// Cluster indicates an expected call of Cluster.
func (mr *MockMongoDBMockRecorder) Cluster(arg0, arg1 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "Cluster", reflect.TypeOf((*MockMongoDB)(nil).Cluster), arg0, arg1)
}

// Clusters mocks base method.
func (m *MockMongoDB) Clusters(arg0 context.Context, arg1 string, arg2, arg3 int64) ([]clusters.ClusterExtended, error) {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "Clusters", arg0, arg1, arg2, arg3)
	ret0, _ := ret[0].([]clusters.ClusterExtended)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// Clusters indicates an expected call of Clusters.
func (mr *MockMongoDBMockRecorder) Clusters(arg0, arg1, arg2, arg3 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "Clusters", reflect.TypeOf((*MockMongoDB)(nil).Clusters), arg0, arg1, arg2, arg3)
}

// CreateCluster mocks base method.
func (m *MockMongoDB) CreateCluster(arg0 context.Context, arg1 mongodb.CreateClusterArgs) (operations.Operation, error) {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "CreateCluster", arg0, arg1)
	ret0, _ := ret[0].(operations.Operation)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// CreateCluster indicates an expected call of CreateCluster.
func (mr *MockMongoDBMockRecorder) CreateCluster(arg0, arg1 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "CreateCluster", reflect.TypeOf((*MockMongoDB)(nil).CreateCluster), arg0, arg1)
}

// CreateDatabase mocks base method.
func (m *MockMongoDB) CreateDatabase(arg0 context.Context, arg1 string, arg2 mongomodels.DatabaseSpec) (operations.Operation, error) {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "CreateDatabase", arg0, arg1, arg2)
	ret0, _ := ret[0].(operations.Operation)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// CreateDatabase indicates an expected call of CreateDatabase.
func (mr *MockMongoDBMockRecorder) CreateDatabase(arg0, arg1, arg2 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "CreateDatabase", reflect.TypeOf((*MockMongoDB)(nil).CreateDatabase), arg0, arg1, arg2)
}

// CreateUser mocks base method.
func (m *MockMongoDB) CreateUser(arg0 context.Context, arg1 string, arg2 mongomodels.UserSpec) (operations.Operation, error) {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "CreateUser", arg0, arg1, arg2)
	ret0, _ := ret[0].(operations.Operation)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// CreateUser indicates an expected call of CreateUser.
func (mr *MockMongoDBMockRecorder) CreateUser(arg0, arg1, arg2 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "CreateUser", reflect.TypeOf((*MockMongoDB)(nil).CreateUser), arg0, arg1, arg2)
}

// Database mocks base method.
func (m *MockMongoDB) Database(arg0 context.Context, arg1, arg2 string) (mongomodels.Database, error) {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "Database", arg0, arg1, arg2)
	ret0, _ := ret[0].(mongomodels.Database)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// Database indicates an expected call of Database.
func (mr *MockMongoDBMockRecorder) Database(arg0, arg1, arg2 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "Database", reflect.TypeOf((*MockMongoDB)(nil).Database), arg0, arg1, arg2)
}

// Databases mocks base method.
func (m *MockMongoDB) Databases(arg0 context.Context, arg1 string, arg2, arg3 int64) ([]mongomodels.Database, error) {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "Databases", arg0, arg1, arg2, arg3)
	ret0, _ := ret[0].([]mongomodels.Database)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// Databases indicates an expected call of Databases.
func (mr *MockMongoDBMockRecorder) Databases(arg0, arg1, arg2, arg3 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "Databases", reflect.TypeOf((*MockMongoDB)(nil).Databases), arg0, arg1, arg2, arg3)
}

// DeleteBackup mocks base method.
func (m *MockMongoDB) DeleteBackup(arg0 context.Context, arg1 string) (operations.Operation, error) {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "DeleteBackup", arg0, arg1)
	ret0, _ := ret[0].(operations.Operation)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// DeleteBackup indicates an expected call of DeleteBackup.
func (mr *MockMongoDBMockRecorder) DeleteBackup(arg0, arg1 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "DeleteBackup", reflect.TypeOf((*MockMongoDB)(nil).DeleteBackup), arg0, arg1)
}

// DeleteCluster mocks base method.
func (m *MockMongoDB) DeleteCluster(arg0 context.Context, arg1 string) (operations.Operation, error) {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "DeleteCluster", arg0, arg1)
	ret0, _ := ret[0].(operations.Operation)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// DeleteCluster indicates an expected call of DeleteCluster.
func (mr *MockMongoDBMockRecorder) DeleteCluster(arg0, arg1 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "DeleteCluster", reflect.TypeOf((*MockMongoDB)(nil).DeleteCluster), arg0, arg1)
}

// DeleteUser mocks base method.
func (m *MockMongoDB) DeleteUser(arg0 context.Context, arg1, arg2 string) (operations.Operation, error) {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "DeleteUser", arg0, arg1, arg2)
	ret0, _ := ret[0].(operations.Operation)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// DeleteUser indicates an expected call of DeleteUser.
func (mr *MockMongoDBMockRecorder) DeleteUser(arg0, arg1, arg2 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "DeleteUser", reflect.TypeOf((*MockMongoDB)(nil).DeleteUser), arg0, arg1, arg2)
}

// ResetupHosts mocks base method.
func (m *MockMongoDB) ResetupHosts(arg0 context.Context, arg1 string, arg2 []string) (operations.Operation, error) {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "ResetupHosts", arg0, arg1, arg2)
	ret0, _ := ret[0].(operations.Operation)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// ResetupHosts indicates an expected call of ResetupHosts.
func (mr *MockMongoDBMockRecorder) ResetupHosts(arg0, arg1, arg2 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "ResetupHosts", reflect.TypeOf((*MockMongoDB)(nil).ResetupHosts), arg0, arg1, arg2)
}

// RestartHosts mocks base method.
func (m *MockMongoDB) RestartHosts(arg0 context.Context, arg1 string, arg2 []string) (operations.Operation, error) {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "RestartHosts", arg0, arg1, arg2)
	ret0, _ := ret[0].(operations.Operation)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// RestartHosts indicates an expected call of RestartHosts.
func (mr *MockMongoDBMockRecorder) RestartHosts(arg0, arg1, arg2 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "RestartHosts", reflect.TypeOf((*MockMongoDB)(nil).RestartHosts), arg0, arg1, arg2)
}

// StepdownHosts mocks base method.
func (m *MockMongoDB) StepdownHosts(arg0 context.Context, arg1 string, arg2 []string) (operations.Operation, error) {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "StepdownHosts", arg0, arg1, arg2)
	ret0, _ := ret[0].(operations.Operation)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// StepdownHosts indicates an expected call of StepdownHosts.
func (mr *MockMongoDBMockRecorder) StepdownHosts(arg0, arg1, arg2 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "StepdownHosts", reflect.TypeOf((*MockMongoDB)(nil).StepdownHosts), arg0, arg1, arg2)
}

// User mocks base method.
func (m *MockMongoDB) User(arg0 context.Context, arg1, arg2 string) (mongomodels.User, error) {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "User", arg0, arg1, arg2)
	ret0, _ := ret[0].(mongomodels.User)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// User indicates an expected call of User.
func (mr *MockMongoDBMockRecorder) User(arg0, arg1, arg2 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "User", reflect.TypeOf((*MockMongoDB)(nil).User), arg0, arg1, arg2)
}

// Users mocks base method.
func (m *MockMongoDB) Users(arg0 context.Context, arg1 string, arg2, arg3 int64) ([]mongomodels.User, error) {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "Users", arg0, arg1, arg2, arg3)
	ret0, _ := ret[0].([]mongomodels.User)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// Users indicates an expected call of Users.
func (mr *MockMongoDBMockRecorder) Users(arg0, arg1, arg2, arg3 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "Users", reflect.TypeOf((*MockMongoDB)(nil).Users), arg0, arg1, arg2, arg3)
}
