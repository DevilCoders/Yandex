package valid_test

import (
	"reflect"
	"testing"
	"time"

	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/library/go/valid"
)

type someCustomType struct{}

func TestEqual(t *testing.T) {
	now := time.Now()

	testCases := []struct {
		value       interface{}
		param       string
		expectedErr error
	}{
		{int(10), "10", nil},
		{int8(78), "78", nil},
		{int16(52), "52", nil},
		{now, now.Format(time.RFC3339Nano), nil},
		{true, "true", nil},
		{"shimba", "shimba", nil},
		{14.42, "55", valid.ErrNotEqual},
		{"ololo", "55", valid.ErrNotEqual},
		{someCustomType{}, "42", valid.ErrInvalidType},
	}

	for _, tc := range testCases {
		t.Run("", func(t *testing.T) {
			err := valid.Equal(reflect.ValueOf(tc.value), tc.param)

			if tc.expectedErr == nil {
				assert.NoError(t, err)
			} else {
				assert.EqualError(t, err, tc.expectedErr.Error())
			}
		})
	}
}

func TestNotEqual(t *testing.T) {
	now := time.Now()

	testCases := []struct {
		value       interface{}
		param       string
		expectedErr error
	}{
		{int(10), "11", nil},
		{int8(78), "79", nil},
		{int16(54), "52", nil},
		{now, now.Add(1 * time.Minute).Format(time.RFC3339Nano), nil},
		{true, "false", nil},
		{"ololo", "trololo", nil},
		{14.42, "14.42", valid.ErrEqual},
		{"ololo", "ololo", valid.ErrEqual},
		{someCustomType{}, "10", valid.ErrInvalidType},
	}

	for _, tc := range testCases {
		t.Run("", func(t *testing.T) {
			err := valid.NotEqual(reflect.ValueOf(tc.value), tc.param)

			if tc.expectedErr == nil {
				assert.NoError(t, err)
			} else {
				assert.EqualError(t, err, tc.expectedErr.Error())
			}
		})
	}
}

func TestMin(t *testing.T) {
	now := time.Now()

	testCases := []struct {
		value       interface{}
		param       string
		expectedErr error
	}{
		{int(14), "10", nil},
		{int8(78), "42", nil},
		{int16(52), "52", nil},
		{now, now.Format(time.RFC3339Nano), nil},
		{now.Add(1 * time.Minute), now.Format(time.RFC3339Nano), nil},
		{14.42, "55.34", valid.ErrLesserValue},
		{someCustomType{}, "10", valid.ErrInvalidType},
		{"ololo", "olol", valid.ErrUnsupportedComparator},
	}

	for _, tc := range testCases {
		t.Run("", func(t *testing.T) {
			err := valid.Min(reflect.ValueOf(tc.value), tc.param)

			if tc.expectedErr == nil {
				assert.NoError(t, err)
			} else {
				assert.EqualError(t, err, tc.expectedErr.Error())
			}
		})
	}
}

func TestMax(t *testing.T) {
	now := time.Now()

	testCases := []struct {
		value       interface{}
		param       string
		expectedErr error
	}{
		{10, "14", nil},
		{int8(42), "78", nil},
		{int16(52), "52", nil},
		{now, now.Format(time.RFC3339Nano), nil},
		{now, now.Add(1 * time.Minute).Format(time.RFC3339Nano), nil},
		{55.14, "14.22", valid.ErrGreaterValue},
		{someCustomType{}, "10", valid.ErrInvalidType},
		{"ololo", "olol", valid.ErrUnsupportedComparator},
	}

	for _, tc := range testCases {
		t.Run("", func(t *testing.T) {
			err := valid.Max(reflect.ValueOf(tc.value), tc.param)

			if tc.expectedErr == nil {
				assert.NoError(t, err)
			} else {
				assert.EqualError(t, err, tc.expectedErr.Error())
			}
		})
	}
}

func TestLesser(t *testing.T) {
	now := time.Now()

	testCases := []struct {
		value       interface{}
		param       string
		expectedErr error
	}{
		{10, "14", nil},
		{int8(42), "78", nil},
		{now, now.Add(1 * time.Minute).Format(time.RFC3339Nano), nil},
		{now, now.Format(time.RFC3339Nano), valid.ErrGreaterValue},
		{int16(52), "52", valid.ErrGreaterValue},
		{55.17, "14.22", valid.ErrGreaterValue},
		{someCustomType{}, "10", valid.ErrInvalidType},
		{"ololo", "olol", valid.ErrUnsupportedComparator},
	}

	for _, tc := range testCases {
		t.Run("", func(t *testing.T) {
			err := valid.Lesser(reflect.ValueOf(tc.value), tc.param)

			if tc.expectedErr == nil {
				assert.NoError(t, err)
			} else {
				assert.EqualError(t, err, tc.expectedErr.Error())
			}
		})
	}
}

func TestGreater(t *testing.T) {
	now := time.Now()

	testCases := []struct {
		value       interface{}
		param       string
		expectedErr error
	}{
		{int(14), "10", nil},
		{int8(78), "42", nil},
		{now.Add(1 * time.Minute), now.Format(time.RFC3339Nano), nil},
		{now, now.Format(time.RFC3339Nano), valid.ErrLesserValue},
		{int16(52), "52", valid.ErrLesserValue},
		{14.42, "55.17", valid.ErrLesserValue},
		{someCustomType{}, "10", valid.ErrInvalidType},
		{"ololo", "olol", valid.ErrUnsupportedComparator},
	}

	for _, tc := range testCases {
		t.Run("", func(t *testing.T) {
			err := valid.Greater(reflect.ValueOf(tc.value), tc.param)

			if tc.expectedErr == nil {
				assert.NoError(t, err)
			} else {
				assert.EqualError(t, err, tc.expectedErr.Error())
			}
		})
	}
}
