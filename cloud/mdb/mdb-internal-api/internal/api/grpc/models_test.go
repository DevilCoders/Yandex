package grpc

import (
	"testing"
	"time"

	"github.com/golang/protobuf/ptypes/wrappers"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
)

func TestOptionalInt64FromGRPC(t *testing.T) {
	nothing := OptionalInt64FromGRPC(nil)
	require.Equal(t, nothing, optional.Int64{})

	zero := OptionalInt64FromGRPC(&wrappers.Int64Value{Value: 0})
	require.Equal(t, zero, optional.NewInt64(0))

	one := OptionalInt64FromGRPC(&wrappers.Int64Value{Value: 1})
	require.Equal(t, one, optional.NewInt64(1))
}

func TestOptionalInt64ToGRPC(t *testing.T) {
	nothing := OptionalInt64ToGRPC(optional.Int64{})
	require.Nil(t, nothing)

	zero := OptionalInt64ToGRPC(optional.NewInt64(0))
	require.True(t, zero != nil && zero.GetValue() == 0)

	one := OptionalInt64ToGRPC(optional.NewInt64(1))
	require.True(t, one != nil && one.GetValue() == 1)
}

func TestOptionalBoolFromGRPC(t *testing.T) {
	nothing := OptionalBoolFromGRPC(nil)
	require.Equal(t, nothing, optional.Bool{})

	zero := OptionalBoolFromGRPC(&wrappers.BoolValue{Value: false})
	require.Equal(t, zero, optional.NewBool(false))

	one := OptionalBoolFromGRPC(&wrappers.BoolValue{Value: true})
	require.Equal(t, one, optional.NewBool(true))
}

func TestOptionalBoolToGRPC(t *testing.T) {
	nothing := OptionalBoolToGRPC(optional.Bool{})
	require.Nil(t, nothing)

	optionalFalse := OptionalBoolToGRPC(optional.NewBool(false))
	require.True(t, optionalFalse != nil && optionalFalse.GetValue() == false)

	optionalTrue := OptionalBoolToGRPC(optional.NewBool(true))
	require.True(t, optionalTrue != nil && optionalTrue.GetValue() == true)
}

func TestOptionalDurationFromGRPC(t *testing.T) {
	nothing := OptionalDurationFromGRPC(nil)
	require.Equal(t, nothing, optional.Duration{})

	zero := OptionalDurationFromGRPC(&wrappers.Int64Value{Value: 0})
	require.True(t, zero.Valid && zero.Must().Milliseconds() == 0)

	oneSecond := OptionalDurationFromGRPC(&wrappers.Int64Value{Value: 1000})
	require.True(t, oneSecond.Valid && oneSecond.Must().Milliseconds() == 1000)
}

func TestOptionalDurationToGRPC(t *testing.T) {
	nothing := OptionalDurationToGRPC(optional.Duration{})
	require.Nil(t, nothing)

	zero := OptionalDurationToGRPC(optional.NewDuration(0))
	require.True(t, zero != nil && zero.GetValue() == 0)

	oneSecond := OptionalDurationToGRPC(optional.NewDuration(time.Second))
	require.True(t, oneSecond != nil && oneSecond.GetValue() == 1000)
}

func TestFieldNamesFromGRPCPaths(t *testing.T) {
	// copypasted from SQLServer config
	type GRPCGeneratedStrut struct {
		MaxDegreeOfParallelism      *wrappers.Int64Value `protobuf:"bytes,1,opt,name=max_degree_of_parallelism,json=maxDegreeOfParallelism,proto3" json:"max_degree_of_parallelism,omitempty"`
		CostThresholdForParallelism *wrappers.Int64Value `protobuf:"bytes,2,opt,name=cost_threshold_for_parallelism,json=costThresholdForParallelism,proto3" json:"cost_threshold_for_parallelism,omitempty"`
		AuditLevel                  *wrappers.Int64Value `protobuf:"bytes,3,opt,name=audit_level,json=auditLevel,proto3" json:"audit_level,omitempty"`
		FillFactorPercent           *wrappers.Int64Value `protobuf:"bytes,4,opt,name=fill_factor_percent,json=fillFactorPercent,proto3" json:"fill_factor_percent,omitempty"`
		OptimizeForAdHocWorkloads   *wrappers.BoolValue  `protobuf:"bytes,5,opt,name=optimize_for_ad_hoc_workloads,json=optimizeForAdHocWorkloads,proto3" json:"optimize_for_ad_hoc_workloads,omitempty"`
	}
	paths := NewFieldPaths([]string{
		"audit_level",
		"max_degree_of_parallelism",
		"foo_bar",
	})
	fields := FieldNamesFromGRPCPaths(&GRPCGeneratedStrut{}, paths)
	require.NotNil(t, fields, "fields slice should not be nil")
	require.Equal(t, []string{"MaxDegreeOfParallelism", "AuditLevel"}, fields, "unexpected fields values")
}
