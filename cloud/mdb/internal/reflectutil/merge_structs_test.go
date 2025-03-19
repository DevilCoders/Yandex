package reflectutil_test

import (
	"testing"

	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/reflectutil"
)

type SomeStruct struct {
	First  string
	Second string
	Third  string
	Fourth int
}

func isNotEmptyString(name string, field interface{}) bool {
	stringField, ok := field.(string)
	if ok {
		return stringField != ""
	}
	return true
}

func TestMerge(t *testing.T) {
	base := &SomeStruct{
		First:  "one",
		Second: "two",
		Third:  "three",
		Fourth: 4,
	}
	add := &SomeStruct{
		First:  "not_one",
		Fourth: 6,
	}
	reflectutil.MergeStructs(base, add, isNotEmptyString)
	expected := &SomeStruct{
		First:  "not_one",
		Second: "two",
		Third:  "three",
		Fourth: 6,
	}
	assert.Equal(t, base, expected)
}

type WithNonExported struct {
	First  int
	Second int
	third  int
}

func TestMergeWithoutNonExported(t *testing.T) {
	base := &WithNonExported{
		First:  1,
		Second: 2,
		third:  3,
	}
	add := &WithNonExported{
		First:  11,
		Second: 0,
		third:  13,
	}
	reflectutil.MergeStructs(base, add, func(name string, field interface{}) bool {
		fieldInt, ok := field.(int)
		if ok {
			return fieldInt != 0
		}
		return true
	})
	assert.Equal(t, base.First, add.First)
	assert.Equal(t, base.Second, 2)
	assert.Equal(t, base.third, 3)
}

func TestMergeArgsIsNotEqualStructsPanicked(t *testing.T) {
	base := &SomeStruct{}
	add := &WithNonExported{}
	require.Panics(t, func() {
		reflectutil.MergeStructs(base, add, alwaysValid)
	})
}

func TestMergeArgsNonStructPanicked(t *testing.T) {
	base := 5
	add := &WithNonExported{}
	require.Panics(t, func() {
		reflectutil.MergeStructs(base, add, alwaysValid)
	})
}

func TestMergeArgsNonPointersPanicked(t *testing.T) {
	base := SomeStruct{
		First:  "one",
		Second: "two",
		Third:  "three",
		Fourth: 4,
	}
	add := SomeStruct{
		First:  "not_one",
		Fourth: 6,
	}
	require.Panics(t, func() {
		reflectutil.MergeStructs(base, add, alwaysValid)
	})
}

type OptStruct struct {
	X optional.Int64
	Y optional.String
	Z string
}

func TestMergeStructsWithOptional(t *testing.T) {
	base := OptStruct{
		X: optional.NewInt64(1),
		Y: optional.NewString("bar"),
		Z: "some",
	}
	add := OptStruct{
		X: optional.NewInt64(2),
		Y: optional.String{Valid: false, String: "badvalue"},
		Z: "other",
	}

	reflectutil.MergeStructs(&base, &add, reflectutil.ValidOptionalFieldsAreValid)
	require.Equal(t, optional.NewInt64(2), base.X, "X field should be updated as valid optional")
	require.Equal(t, optional.NewString("bar"), base.Y, "Y field should NOT be updated as invalid optional")
	require.Equal(t, "some", base.Z, "Z field should not be updated as NOT optional")
}

func TestMergeStructsByFieldNames(t *testing.T) {
	base := OptStruct{
		X: optional.NewInt64(1),
		Y: optional.NewString("bar"),
		Z: "some",
	}
	add := OptStruct{
		X: optional.NewInt64(2),
		Y: optional.String{},
		Z: "other",
	}
	toUpdate := []string{"Y", "Z", "Lol"}

	reflectutil.MergeStructs(&base, &add, reflectutil.FieldsInListAreValid(toUpdate))
	require.Equal(t, optional.NewInt64(1), base.X, "X field should NOT be updated as not in list")
	require.Equal(t, optional.String{}, base.Y, "Y field should be updated")
	require.Equal(t, "other", base.Z, "Z field should not be updated")
}
