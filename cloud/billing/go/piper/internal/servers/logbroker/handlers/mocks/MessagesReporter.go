// Code generated by mockery. DO NOT EDIT.

package mocks

import mock "github.com/stretchr/testify/mock"

// MessagesReporter is an autogenerated mock type for the MessagesReporter type
type MessagesReporter struct {
	mock.Mock
}

// Consumed provides a mock function with given fields:
func (_m *MessagesReporter) Consumed() {
	_m.Called()
}

// Error provides a mock function with given fields: _a0
func (_m *MessagesReporter) Error(_a0 error) {
	_m.Called(_a0)
}
