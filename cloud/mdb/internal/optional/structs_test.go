package optional_test

import (
	"testing"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
)

type BaseStruct struct {
	String      string
	Int64       int64
	Bool        bool
	IntPointer  *int64
	BoolPointer *bool
}

type BaseStructWithoutField struct {
	Int64 int64
	Bool  bool
}

type UpdateStruct struct {
	String      optional.String
	Int64       optional.Int64
	Bool        optional.Bool
	IntPointer  optional.Int64Pointer
	BoolPointer optional.BoolPointer
}

func TestApplyUpdateAllowsToUpdateFieldOfBaseStruct(t *testing.T) {
	intValBase := int64(3)
	boolValBase := false
	base := &BaseStruct{
		String:      "old",
		Int64:       int64(1),
		Bool:        true,
		IntPointer:  &intValBase,
		BoolPointer: &boolValBase,
	}
	intValUpdated := int64(4)
	boolValUpdated := true
	update := &UpdateStruct{
		String:      optional.NewString("updated"),
		Int64:       optional.NewInt64(2),
		Bool:        optional.NewBool(false),
		IntPointer:  optional.NewInt64Pointer(&intValUpdated),
		BoolPointer: optional.NewBoolPointer(&boolValUpdated),
	}
	hasChanges := optional.ApplyUpdate(base, update)
	require.True(t, hasChanges)
	require.Equal(t, "updated", base.String)
	require.Equal(t, int64(2), base.Int64)
	require.Equal(t, int64(4), *base.IntPointer)
	require.Equal(t, true, *base.BoolPointer)
	require.False(t, base.Bool)
}

func TestApplyUpdateReportsIfNoChangesFound(t *testing.T) {
	intValBase := int64(3)
	boolValBase := false
	base := &BaseStruct{
		String:      "one",
		Int64:       int64(1),
		Bool:        true,
		IntPointer:  &intValBase,
		BoolPointer: &boolValBase,
	}
	intValUpdated := int64(3)
	boolValUpdated := false
	update := &UpdateStruct{
		Int64:       optional.NewInt64(1),
		Bool:        optional.NewBool(true),
		IntPointer:  optional.NewInt64Pointer(&intValUpdated),
		BoolPointer: optional.NewBoolPointer(&boolValUpdated),
	}
	hasChanges := optional.ApplyUpdate(base, update)
	require.False(t, hasChanges)
}

func TestApplyUpdateAllowsToSetDefaultTypeValues(t *testing.T) {
	intValBase := int64(3)
	boolValBase := false
	base := &BaseStruct{
		String:      "one",
		Int64:       int64(1),
		Bool:        true,
		IntPointer:  &intValBase,
		BoolPointer: &boolValBase,
	}
	update := &UpdateStruct{
		String:      optional.NewString(""),
		Int64:       optional.NewInt64(0),
		Bool:        optional.NewBool(false),
		IntPointer:  optional.NewInt64Pointer(nil),
		BoolPointer: optional.NewBoolPointer(nil),
	}
	hasChanges := optional.ApplyUpdate(base, update)
	require.True(t, hasChanges)
	require.Equal(t, "", base.String)
	require.Equal(t, int64(0), base.Int64)
	require.Equal(t, (*int64)(nil), base.IntPointer)
	require.False(t, base.Bool)
	require.Equal(t, (*bool)(nil), base.BoolPointer)
}

func TestApplyUpdateRequiresBaseToBePassedByPointer(t *testing.T) {
	base := BaseStruct{}
	update := &UpdateStruct{}
	require.PanicsWithValue(t, "base argument is not a pointer", func() {
		optional.ApplyUpdate(base, update)
	})
}

func TestApplyUpdateWorksWithStructsOnly(t *testing.T) {
	base := 1
	update := &UpdateStruct{}
	require.PanicsWithValue(t, "provided base value is not a struct: 1", func() {
		optional.ApplyUpdate(&base, update)
	})
}

func TestApplyUpdateReportsMissingBaseField(t *testing.T) {
	base := &BaseStructWithoutField{}
	update := &UpdateStruct{
		String: optional.NewString("updated"),
	}
	require.PanicsWithValue(t, "field \"String\" is present within update type UpdateStruct"+
		" but not found within base type BaseStructWithoutField", func() {
		optional.ApplyUpdate(base, update)
	})
}
