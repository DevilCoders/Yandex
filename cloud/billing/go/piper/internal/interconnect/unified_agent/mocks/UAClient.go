// Code generated by mockery v2.10.0. DO NOT EDIT.

package mocks

import (
	context "context"

	unifiedagent "a.yandex-team.ru/cloud/billing/go/piper/internal/interconnect/unified_agent"
	mock "github.com/stretchr/testify/mock"
)

// UAClient is an autogenerated mock type for the UAClient type
type UAClient struct {
	mock.Mock
}

// HealthCheck provides a mock function with given fields: _a0
func (_m *UAClient) HealthCheck(_a0 context.Context) error {
	ret := _m.Called(_a0)

	var r0 error
	if rf, ok := ret.Get(0).(func(context.Context) error); ok {
		r0 = rf(_a0)
	} else {
		r0 = ret.Error(0)
	}

	return r0
}

// PushMetrics provides a mock function with given fields: ctx, metrics
func (_m *UAClient) PushMetrics(ctx context.Context, metrics ...unifiedagent.SolomonMetric) error {
	_va := make([]interface{}, len(metrics))
	for _i := range metrics {
		_va[_i] = metrics[_i]
	}
	var _ca []interface{}
	_ca = append(_ca, ctx)
	_ca = append(_ca, _va...)
	ret := _m.Called(_ca...)

	var r0 error
	if rf, ok := ret.Get(0).(func(context.Context, ...unifiedagent.SolomonMetric) error); ok {
		r0 = rf(ctx, metrics...)
	} else {
		r0 = ret.Error(0)
	}

	return r0
}
