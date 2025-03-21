// Code generated by mockery v2.10.0. DO NOT EDIT.

package mocks

import (
	context "context"

	domain "a.yandex-team.ru/cloud/dataplatform/schemaregistry/internal/server/domain"
	mock "github.com/stretchr/testify/mock"
)

// SchemaService is an autogenerated mock type for the SchemaService type
type SchemaService struct {
	mock.Mock
}

func (_m *SchemaService) Diff(ctx context.Context, namespace string, schema string, data []byte) ([]string, error) {
	//TODO implement me
	panic("implement me")
}

// CheckCompatibility provides a mock function with given fields: _a0, _a1, _a2, _a3, _a4
func (_m *SchemaService) CheckCompatibility(_a0 context.Context, _a1 string, _a2 string, _a3 string, _a4 []byte) error {
	ret := _m.Called(_a0, _a1, _a2, _a3, _a4)

	var r0 error
	if rf, ok := ret.Get(0).(func(context.Context, string, string, string, []byte) error); ok {
		r0 = rf(_a0, _a1, _a2, _a3, _a4)
	} else {
		r0 = ret.Error(0)
	}

	return r0
}

// Create provides a mock function with given fields: _a0, _a1, _a2, _a3, _a4
func (_m *SchemaService) Create(_a0 context.Context, _a1 string, _a2 string, _a3 *domain.Metadata, _a4 []byte) (domain.SchemaInfo, error) {
	ret := _m.Called(_a0, _a1, _a2, _a3, _a4)

	var r0 domain.SchemaInfo
	if rf, ok := ret.Get(0).(func(context.Context, string, string, *domain.Metadata, []byte) domain.SchemaInfo); ok {
		r0 = rf(_a0, _a1, _a2, _a3, _a4)
	} else {
		r0 = ret.Get(0).(domain.SchemaInfo)
	}

	var r1 error
	if rf, ok := ret.Get(1).(func(context.Context, string, string, *domain.Metadata, []byte) error); ok {
		r1 = rf(_a0, _a1, _a2, _a3, _a4)
	} else {
		r1 = ret.Error(1)
	}

	return r0, r1
}

// Delete provides a mock function with given fields: _a0, _a1, _a2
func (_m *SchemaService) Delete(_a0 context.Context, _a1 string, _a2 string) error {
	ret := _m.Called(_a0, _a1, _a2)

	var r0 error
	if rf, ok := ret.Get(0).(func(context.Context, string, string) error); ok {
		r0 = rf(_a0, _a1, _a2)
	} else {
		r0 = ret.Error(0)
	}

	return r0
}

// DeleteVersion provides a mock function with given fields: _a0, _a1, _a2, _a3
func (_m *SchemaService) DeleteVersion(_a0 context.Context, _a1 string, _a2 string, _a3 int32) error {
	ret := _m.Called(_a0, _a1, _a2, _a3)

	var r0 error
	if rf, ok := ret.Get(0).(func(context.Context, string, string, int32) error); ok {
		r0 = rf(_a0, _a1, _a2, _a3)
	} else {
		r0 = ret.Error(0)
	}

	return r0
}

// Get provides a mock function with given fields: _a0, _a1, _a2, _a3
func (_m *SchemaService) Get(_a0 context.Context, _a1 string, _a2 string, _a3 int32) (*domain.Metadata, []byte, error) {
	ret := _m.Called(_a0, _a1, _a2, _a3)

	var r0 *domain.Metadata
	if rf, ok := ret.Get(0).(func(context.Context, string, string, int32) *domain.Metadata); ok {
		r0 = rf(_a0, _a1, _a2, _a3)
	} else {
		if ret.Get(0) != nil {
			r0 = ret.Get(0).(*domain.Metadata)
		}
	}

	var r1 []byte
	if rf, ok := ret.Get(1).(func(context.Context, string, string, int32) []byte); ok {
		r1 = rf(_a0, _a1, _a2, _a3)
	} else {
		if ret.Get(1) != nil {
			r1 = ret.Get(1).([]byte)
		}
	}

	var r2 error
	if rf, ok := ret.Get(2).(func(context.Context, string, string, int32) error); ok {
		r2 = rf(_a0, _a1, _a2, _a3)
	} else {
		r2 = ret.Error(2)
	}

	return r0, r1, r2
}

// GetLatest provides a mock function with given fields: _a0, _a1, _a2
func (_m *SchemaService) GetLatest(_a0 context.Context, _a1 string, _a2 string) (*domain.Metadata, []byte, error) {
	ret := _m.Called(_a0, _a1, _a2)

	var r0 *domain.Metadata
	if rf, ok := ret.Get(0).(func(context.Context, string, string) *domain.Metadata); ok {
		r0 = rf(_a0, _a1, _a2)
	} else {
		if ret.Get(0) != nil {
			r0 = ret.Get(0).(*domain.Metadata)
		}
	}

	var r1 []byte
	if rf, ok := ret.Get(1).(func(context.Context, string, string) []byte); ok {
		r1 = rf(_a0, _a1, _a2)
	} else {
		if ret.Get(1) != nil {
			r1 = ret.Get(1).([]byte)
		}
	}

	var r2 error
	if rf, ok := ret.Get(2).(func(context.Context, string, string) error); ok {
		r2 = rf(_a0, _a1, _a2)
	} else {
		r2 = ret.Error(2)
	}

	return r0, r1, r2
}

// GetMetadata provides a mock function with given fields: _a0, _a1, _a2
func (_m *SchemaService) GetMetadata(_a0 context.Context, _a1 string, _a2 string) (*domain.Metadata, error) {
	ret := _m.Called(_a0, _a1, _a2)

	var r0 *domain.Metadata
	if rf, ok := ret.Get(0).(func(context.Context, string, string) *domain.Metadata); ok {
		r0 = rf(_a0, _a1, _a2)
	} else {
		if ret.Get(0) != nil {
			r0 = ret.Get(0).(*domain.Metadata)
		}
	}

	var r1 error
	if rf, ok := ret.Get(1).(func(context.Context, string, string) error); ok {
		r1 = rf(_a0, _a1, _a2)
	} else {
		r1 = ret.Error(1)
	}

	return r0, r1
}

// List provides a mock function with given fields: _a0, _a1
func (_m *SchemaService) List(_a0 context.Context, _a1 string) ([]string, error) {
	ret := _m.Called(_a0, _a1)

	var r0 []string
	if rf, ok := ret.Get(0).(func(context.Context, string) []string); ok {
		r0 = rf(_a0, _a1)
	} else {
		if ret.Get(0) != nil {
			r0 = ret.Get(0).([]string)
		}
	}

	var r1 error
	if rf, ok := ret.Get(1).(func(context.Context, string) error); ok {
		r1 = rf(_a0, _a1)
	} else {
		r1 = ret.Error(1)
	}

	return r0, r1
}

// ListVersions provides a mock function with given fields: _a0, _a1, _a2
func (_m *SchemaService) ListVersions(_a0 context.Context, _a1 string, _a2 string) ([]int32, error) {
	ret := _m.Called(_a0, _a1, _a2)

	var r0 []int32
	if rf, ok := ret.Get(0).(func(context.Context, string, string) []int32); ok {
		r0 = rf(_a0, _a1, _a2)
	} else {
		if ret.Get(0) != nil {
			r0 = ret.Get(0).([]int32)
		}
	}

	var r1 error
	if rf, ok := ret.Get(1).(func(context.Context, string, string) error); ok {
		r1 = rf(_a0, _a1, _a2)
	} else {
		r1 = ret.Error(1)
	}

	return r0, r1
}

// UpdateMetadata provides a mock function with given fields: _a0, _a1, _a2, _a3
func (_m *SchemaService) UpdateMetadata(_a0 context.Context, _a1 string, _a2 string, _a3 *domain.Metadata) (*domain.Metadata, error) {
	ret := _m.Called(_a0, _a1, _a2, _a3)

	var r0 *domain.Metadata
	if rf, ok := ret.Get(0).(func(context.Context, string, string, *domain.Metadata) *domain.Metadata); ok {
		r0 = rf(_a0, _a1, _a2, _a3)
	} else {
		if ret.Get(0) != nil {
			r0 = ret.Get(0).(*domain.Metadata)
		}
	}

	var r1 error
	if rf, ok := ret.Get(1).(func(context.Context, string, string, *domain.Metadata) error); ok {
		r1 = rf(_a0, _a1, _a2, _a3)
	} else {
		r1 = ret.Error(1)
	}

	return r0, r1
}
