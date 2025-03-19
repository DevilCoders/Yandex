package reflectutil_test

import (
	"testing"

	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/reflectutil"
)

func alwaysValid(name string, field interface{}) bool {
	return true
}

var defaultCopyStructFieldsConfig = reflectutil.CopyStructFieldsConfig{}

type SimpleTestStructOriginal struct {
	First  int
	Second string
}

type SimpleTestStructCopy struct {
	First  int
	Second string
	Third  bool
}

type SameAsOriginal struct {
	First  int
	Second string
}

func TestCopyFieldsSimpleStruct(t *testing.T) {
	src := &SimpleTestStructOriginal{First: 14, Second: "something"}
	dst := &SimpleTestStructCopy{Third: false}
	reflectutil.CopyStructFields(src, dst, defaultCopyStructFieldsConfig)
	assert.Equal(t, dst.First, src.First)
	assert.Equal(t, dst.Second, src.Second)
	assert.Equal(t, dst.Third, false)
}

type NestedTestStructOriginal struct {
	Simple int
	Nest   SimpleTestStructOriginal
}

type NestedTestStructCopy struct {
	Simple int
	Nest   SimpleTestStructOriginal
	Third  bool
}

func TestCopyFieldsNestedStruct(t *testing.T) {
	simple := SimpleTestStructOriginal{First: 14, Second: "something"}
	src := &NestedTestStructOriginal{Simple: 14, Nest: simple}
	dst := &NestedTestStructCopy{Third: false}
	reflectutil.CopyStructFields(src, dst, defaultCopyStructFieldsConfig)
	assert.Equal(t, dst.Simple, src.Simple)
	assert.Equal(t, dst.Nest, src.Nest)
	assert.Equal(t, dst.Third, false)
}

type Unexported struct {
	First  int
	second string
}

type UnexportedCopy struct {
	First  int
	second string
	Third  bool
}

func TestCopyFieldsWithoutUnexported(t *testing.T) {
	src := &Unexported{First: 14, second: "something"}
	dst := &UnexportedCopy{Third: false, second: "nothing"}
	reflectutil.CopyStructFields(src, dst, defaultCopyStructFieldsConfig)
	assert.Equal(t, dst.First, src.First)
	assert.Equal(t, dst.second, "nothing")
	assert.Equal(t, dst.Third, false)
}

func TestCopyFieldsArgsNonStructPanicked(t *testing.T) {
	base := 5
	add := &Unexported{}
	require.Panics(t, func() {
		reflectutil.MergeStructs(base, add, func(string, interface{}) bool { return true })
	})
}

func TestCopyFieldsArgsNonPointersPanicked(t *testing.T) {
	src := SimpleTestStructOriginal{First: 14, Second: "something"}
	dst := SimpleTestStructCopy{Third: false}
	require.Panics(t, func() {
		reflectutil.CopyStructFields(src, dst, defaultCopyStructFieldsConfig)
	})
}

func TestStrictSuccessfulCopy(t *testing.T) {
	src := &SimpleTestStructOriginal{First: 14, Second: "something"}
	dst := &SameAsOriginal{}
	reflectutil.CopyStructFieldsStrict(src, dst)
	require.Equal(t, 14, dst.First)
	require.Equal(t, "something", dst.Second)
}

func TestStrictMissingFromSource(t *testing.T) {
	src := &SimpleTestStructOriginal{First: 14, Second: "something"}
	dst := &SimpleTestStructCopy{}
	msg := "field \"Third\" is present within destination struct of type SimpleTestStructCopy" +
		" but missing from source struct of type SimpleTestStructOriginal"
	require.PanicsWithValue(t, msg, func() {
		reflectutil.CopyStructFieldsStrict(src, dst)
	})
}

func TestStrictMissingFromDestination(t *testing.T) {
	src := &SimpleTestStructCopy{First: 14, Second: "something", Third: true}
	dst := &SimpleTestStructOriginal{}
	msg := "field \"Third\" is present within source struct of type SimpleTestStructCopy " +
		"but missing from destination struct of type SimpleTestStructOriginal"
	require.PanicsWithValue(t, msg, func() {
		reflectutil.CopyStructFieldsStrict(src, dst)
	})
}
