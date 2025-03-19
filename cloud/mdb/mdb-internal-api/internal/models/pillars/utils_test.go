package pillars

import (
	"testing"
	"time"

	fieldmask_utils "github.com/mennanov/fieldmask-utils"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
)

func TestMapOptionalInt64ToPtrInt64(t *testing.T) {
	nothing := MapOptionalInt64ToPtrInt64(optional.Int64{})
	require.Nil(t, nothing)

	zero := MapOptionalInt64ToPtrInt64(optional.NewInt64(0))
	require.True(t, zero != nil && *zero == 0)

	one := MapOptionalInt64ToPtrInt64(optional.NewInt64(1))
	require.True(t, one != nil && *one == 1)
}

func TestMapPtrInt64ToOptionalInt64(t *testing.T) {
	nothing := MapPtrInt64ToOptionalInt64(nil)
	require.Equal(t, nothing, optional.Int64{})

	var zeroValue int64
	zero := MapPtrInt64ToOptionalInt64(&zeroValue)
	require.Equal(t, zero, optional.NewInt64(0))

	oneValue := int64(1)
	one := MapPtrInt64ToOptionalInt64(&oneValue)
	require.Equal(t, one, optional.NewInt64(1))
}

func TestMapOptionalBoolToPtrInt64(t *testing.T) {
	nothing := MapOptionalBoolToPtrInt64(optional.Bool{})
	require.Nil(t, nothing)

	zero := MapOptionalBoolToPtrInt64(optional.NewBool(false))
	require.True(t, zero != nil && *zero == 0)

	one := MapOptionalBoolToPtrInt64(optional.NewBool(true))
	require.True(t, one != nil && *one == 1)
}

func TestMapPtrInt64ToOptionalBool(t *testing.T) {
	nothing := MapPtrInt64ToOptionalBool(nil)
	require.Equal(t, nothing, optional.Bool{})

	var zeroValue int64
	zero := MapPtrInt64ToOptionalBool(&zeroValue)
	require.Equal(t, zero, optional.NewBool(false))

	oneValue := int64(1)
	one := MapPtrInt64ToOptionalBool(&oneValue)
	require.Equal(t, one, optional.NewBool(true))
}

func TestMapOptionalDurationToPtrSeconds(t *testing.T) {
	nothing := MapOptionalDurationToPtrSeconds(optional.Duration{})
	require.Nil(t, nothing)

	zero := MapOptionalDurationToPtrSeconds(optional.NewDuration(0))
	require.True(t, zero != nil && *zero == 0)

	oneSecond := MapOptionalDurationToPtrSeconds(optional.NewDuration(time.Second))
	require.True(t, oneSecond != nil && *oneSecond == 1)
}

func TestMapPtrSecondsToOptionalDuration(t *testing.T) {
	nothing := MapPtrSecondsToOptionalDuration(nil)
	require.Equal(t, nothing, optional.Duration{})

	var zeroValue int64
	zero := MapPtrSecondsToOptionalDuration(&zeroValue)
	require.Equal(t, zero, optional.NewDuration(0))

	oneSecondValue := int64(1)
	oneSecond := MapPtrSecondsToOptionalDuration(&oneSecondValue)
	require.True(t, oneSecond.Valid && oneSecond.Must().Milliseconds() == 1000)
}

func TestMapModelToPillar(t *testing.T) {
	t.Run("one bool", func(t *testing.T) {
		model := struct {
			Val optional.Bool
		}{Val: optional.NewBool(true)}
		pillar := struct {
			Val *bool
		}{}
		require.NoError(t, MapModelToPillar(&model, &pillar))
		require.True(t, *pillar.Val)
	})
	t.Run("one int64", func(t *testing.T) {
		model := struct {
			Val optional.Int64
		}{Val: optional.NewInt64(42)}
		pillar := struct {
			Val *int64
		}{}
		require.NoError(t, MapModelToPillar(&model, &pillar))
		require.Equal(t, *pillar.Val, int64(42))
	})
	t.Run("one string", func(t *testing.T) {
		model := struct {
			Val optional.String
		}{Val: optional.NewString("some_string")}
		pillar := struct {
			Val *string
		}{}
		require.NoError(t, MapModelToPillar(&model, &pillar))
		require.Equal(t, *pillar.Val, "some_string")
	})
	t.Run("one duration", func(t *testing.T) {
		model := struct {
			Val optional.Duration
		}{Val: optional.NewDuration(time.Minute * 7)}
		pillar := struct {
			Val *time.Duration
		}{}
		require.NoError(t, MapModelToPillar(&model, &pillar))
		require.Equal(t, *pillar.Val, time.Minute*7)
	})
	t.Run("skip unexported", func(t *testing.T) {
		model := struct {
			A optional.Int64
			b optional.Int64
			C optional.Int64
		}{optional.NewInt64(1), optional.NewInt64(2), optional.NewInt64(3)}
		pillar := struct {
			A *int64
			b *int64
			C *int64
		}{}
		require.NoError(t, MapModelToPillar(&model, &pillar))
		require.Equal(t, *pillar.A, int64(1))
		require.Equal(t, pillar.b, (*int64)(nil))
		require.Equal(t, *pillar.C, int64(3))
	})
	t.Run("not struct pointer model fails", func(t *testing.T) {
		pillar := struct{}{}
		require.Error(t, MapModelToPillar(1, &pillar))
	})
	t.Run("not struct pointer pillar fails", func(t *testing.T) {
		model := struct{}{}
		require.Error(t, MapModelToPillar(model, 1))
	})
	t.Run("different schema works", func(t *testing.T) {
		model := struct {
			A optional.Int64
			b optional.Int64
			C optional.Int64
		}{optional.NewInt64(1), optional.NewInt64(2), optional.NewInt64(3)}
		pillar := struct {
			A *int64
			b *int64
		}{}
		require.NoError(t, MapModelToPillar(&model, &pillar))
		require.Equal(t, *pillar.A, int64(1))
		require.Equal(t, pillar.b, (*int64)(nil))
	})
	t.Run("non-optional works", func(t *testing.T) {
		model := struct {
			A int64
			B time.Duration
			C string
			D bool
		}{1, time.Minute, "string_val", true}
		pillar := struct {
			A *int64
			B *time.Duration
			C *string
			D *bool
		}{}
		require.NoError(t, MapModelToPillar(&model, &pillar))
		require.Equal(t, *pillar.A, int64(1))
		require.Equal(t, *pillar.B, time.Minute)
		require.Equal(t, *pillar.C, "string_val")
		require.Equal(t, *pillar.D, true)
	})
	t.Run("to non-pointer works", func(t *testing.T) {
		model := struct {
			A optional.Int64
			B optional.Duration
			C optional.String
			D optional.Bool
		}{
			optional.NewInt64(1),
			optional.NewDuration(time.Minute),
			optional.NewString("string_val"),
			optional.NewBool(true),
		}
		pillar := struct {
			A int64
			B time.Duration
			C string
			D bool
		}{}
		require.NoError(t, MapModelToPillar(&model, &pillar))
		require.Equal(t, pillar.A, int64(1))
		require.Equal(t, pillar.B, time.Minute)
		require.Equal(t, pillar.C, "string_val")
		require.Equal(t, pillar.D, true)
	})
	t.Run("omit fields", func(t *testing.T) {
		model := struct {
			A optional.Int64
			B []string        `mapping:"omit"`
			C optional.String `mapping:"omit"`
			D optional.Bool
		}{
			optional.NewInt64(1),
			[]string{"str"},
			optional.NewString("string_val"),
			optional.NewBool(true),
		}
		pillar := struct {
			A int64
			B time.Duration
			C string
			D bool
		}{}
		require.NoError(t, MapModelToPillar(&model, &pillar))
		require.Equal(t, pillar.A, int64(1))
		require.Equal(t, pillar.B, time.Duration(0))
		require.Equal(t, pillar.C, "")
		require.Equal(t, pillar.D, true)
	})
	t.Run("map subfields", func(t *testing.T) {
		type sub struct {
			C int64
			D string
		}

		model := struct {
			A string
			B sub `mapping:"map"`
		}{
			A: "str",
			B: sub{
				C: 77,
				D: "substr",
			},
		}
		pillar := struct {
			A *string
			B struct {
				C *int64
				D *string
			}
		}{}
		require.NoError(t, MapModelToPillar(&model, &pillar))
		require.NotNil(t, pillar.A)
		require.Equal(t, *pillar.A, "str")
		require.NotNil(t, pillar.B.C)
		require.Equal(t, *pillar.B.C, int64(77))
		require.NotNil(t, pillar.B.D)
		require.Equal(t, *pillar.B.D, "substr")
	})
}

func TestMapPillarToModel(t *testing.T) {
	t.Run("one bool", func(t *testing.T) {
		model := struct {
			Val optional.Bool
		}{}
		pillar := struct {
			Val *bool
		}{MapOptionalBoolToPtrBool(optional.NewBool(true))}
		require.NoError(t, MapPillarToModel(&pillar, &model))
		require.True(t, model.Val.Valid)
		require.Equal(t, model.Val.Bool, true)
	})
	t.Run("one int64", func(t *testing.T) {
		model := struct {
			Val optional.Int64
		}{}
		pillar := struct {
			Val *int64
		}{MapOptionalInt64ToPtrInt64(optional.NewInt64(42))}
		require.NoError(t, MapPillarToModel(&pillar, &model))
		require.True(t, model.Val.Valid)
		require.Equal(t, model.Val.Int64, int64(42))
	})
	t.Run("one string", func(t *testing.T) {
		model := struct {
			Val optional.String
		}{}
		pillar := struct {
			Val *string
		}{MapOptionalStringToPtrString(optional.NewString("string_val"))}
		require.NoError(t, MapPillarToModel(&pillar, &model))
		require.True(t, model.Val.Valid)
		require.Equal(t, model.Val.String, "string_val")
	})
	t.Run("one duration", func(t *testing.T) {
		model := struct {
			Val optional.Duration
		}{}
		pillar := struct {
			Val *time.Duration
		}{MapOptionalDurationToPtrDuration(optional.NewDuration(time.Minute))}
		require.NoError(t, MapPillarToModel(&pillar, &model))
		require.True(t, model.Val.Valid)
		require.Equal(t, model.Val.Duration, time.Minute)
	})
	t.Run("skip unexported", func(t *testing.T) {
		model := struct {
			A optional.Int64
			b optional.Int64
			C optional.Int64
		}{}
		pillar := struct {
			A *int64
			b *int64
			C *int64
		}{
			MapOptionalInt64ToPtrInt64(optional.NewInt64(1)),
			MapOptionalInt64ToPtrInt64(optional.NewInt64(2)),
			MapOptionalInt64ToPtrInt64(optional.NewInt64(3)),
		}
		require.NoError(t, MapPillarToModel(&pillar, &model))
		require.True(t, model.A.Valid)
		require.False(t, model.b.Valid)
		require.True(t, model.C.Valid)

		require.Equal(t, model.A.Int64, int64(1))
		require.Equal(t, model.b.Int64, int64(0))
		require.Equal(t, model.C.Int64, int64(3))
	})
	t.Run("not struct pointer pillar fails", func(t *testing.T) {
		pillar := struct{}{}
		require.Error(t, MapPillarToModel(1, &pillar))
	})
	t.Run("not struct pointer model fails", func(t *testing.T) {
		model := struct{}{}
		require.Error(t, MapPillarToModel(model, 1))
	})
	t.Run("different schema works", func(t *testing.T) {
		model := struct {
			A optional.Int64
			b optional.Int64
		}{}
		pillar := struct {
			A *int64
			b *int64
			C *int64
		}{
			MapOptionalInt64ToPtrInt64(optional.NewInt64(1)),
			MapOptionalInt64ToPtrInt64(optional.NewInt64(2)),
			MapOptionalInt64ToPtrInt64(optional.NewInt64(3)),
		}
		require.NoError(t, MapPillarToModel(&pillar, &model))
		require.True(t, model.A.Valid)
		require.False(t, model.b.Valid)

		require.Equal(t, model.A.Int64, int64(1))
		require.Equal(t, model.b.Int64, int64(0))
	})
	t.Run("non-optional works", func(t *testing.T) {
		model := struct {
			A int64
			B time.Duration
			C string
			D bool
		}{}
		pillar := struct {
			A *int64
			B *time.Duration
			C *string
			D *bool
		}{MapOptionalInt64ToPtrInt64(optional.NewInt64(1)),
			MapOptionalDurationToPtrDuration(optional.NewDuration(time.Minute)),
			MapOptionalStringToPtrString(optional.NewString("string_val")),
			MapOptionalBoolToPtrBool(optional.NewBool(true)),
		}
		require.NoError(t, MapPillarToModel(&pillar, &model))
		require.Equal(t, model.A, *pillar.A)
		require.Equal(t, model.B, *pillar.B)
		require.Equal(t, model.C, *pillar.C)
		require.Equal(t, model.D, *pillar.D)
	})
	t.Run("from non-pointer works", func(t *testing.T) {
		model := struct {
			A optional.Int64
			B optional.Duration
			C optional.String
			D optional.Bool
		}{}
		pillar := struct {
			A int64
			B time.Duration
			C string
			D bool
		}{1, time.Minute, "string_val", true}
		require.NoError(t, MapPillarToModel(&pillar, &model))
		require.Equal(t, model.A, optional.NewInt64(int64(1)))
		require.Equal(t, model.B, optional.NewDuration(time.Minute))
		require.Equal(t, model.C, optional.NewString("string_val"))
		require.Equal(t, model.D, optional.NewBool(true))
	})
}

func TestUpdatePillarByFieldMask(t *testing.T) {
	type innerStruct struct {
		ValA *int64 `name:"val_a"`
		ValB *int64 `name:"val_b"`
	}

	type pillar struct {
		Int          *int64         `name:"int"`
		Time         *time.Duration `name:"time"`
		String       *string        `name:"string"`
		Bool         *bool          `name:"bool"`
		ComplexField innerStruct    `name:"complex_field"`
	}

	t.Run("update all", func(t *testing.T) {
		currentPillar := pillar{}
		updatePillar := pillar{
			Int:    MapOptionalInt64ToPtrInt64(optional.NewInt64(7)),
			Time:   MapOptionalDurationToPtrDuration(optional.NewDuration(time.Minute)),
			String: MapOptionalStringToPtrString(optional.NewString("string_val")),
			Bool:   MapOptionalBoolToPtrBool(optional.NewBool(true)),
			ComplexField: innerStruct{
				ValA: MapOptionalInt64ToPtrInt64(optional.NewInt64(77)),
				ValB: MapOptionalInt64ToPtrInt64(optional.NewInt64(777)),
			},
		}
		require.NoError(t, UpdatePillarByFieldMask(&currentPillar, &updatePillar, fieldmask_utils.Mask{}))
		require.Equal(t, currentPillar, updatePillar)
	})
	t.Run("update subset", func(t *testing.T) {
		currentPillar := pillar{}
		updatePillar := pillar{
			Int:    MapOptionalInt64ToPtrInt64(optional.NewInt64(7)),
			Time:   MapOptionalDurationToPtrDuration(optional.NewDuration(time.Minute)),
			String: MapOptionalStringToPtrString(optional.NewString("string_val")),
			Bool:   MapOptionalBoolToPtrBool(optional.NewBool(true)),
			ComplexField: innerStruct{
				ValA: MapOptionalInt64ToPtrInt64(optional.NewInt64(77)),
				ValB: MapOptionalInt64ToPtrInt64(optional.NewInt64(777)),
			},
		}
		mask, err := fieldmask_utils.MaskFromPaths([]string{
			"int", "string", "complex_field",
		}, func(s string) string { return s })
		require.NoError(t, err)
		require.NoError(t, UpdatePillarByFieldMask(&currentPillar, &updatePillar, mask))
		require.Equal(t, currentPillar.Int, updatePillar.Int)
		require.Equal(t, currentPillar.String, updatePillar.String)
		require.Equal(t, currentPillar.ComplexField, updatePillar.ComplexField)
		require.Nil(t, currentPillar.Time)
		require.Nil(t, currentPillar.Bool)
	})
	t.Run("unset values by filter", func(t *testing.T) {
		updatePillar := pillar{}
		currentPillar := pillar{
			Int:    MapOptionalInt64ToPtrInt64(optional.NewInt64(7)),
			Time:   MapOptionalDurationToPtrDuration(optional.NewDuration(time.Minute)),
			String: MapOptionalStringToPtrString(optional.NewString("string_val")),
			Bool:   MapOptionalBoolToPtrBool(optional.NewBool(true)),
			ComplexField: innerStruct{
				ValA: MapOptionalInt64ToPtrInt64(optional.NewInt64(77)),
				ValB: MapOptionalInt64ToPtrInt64(optional.NewInt64(777)),
			},
		}
		mask, err := fieldmask_utils.MaskFromPaths([]string{
			"time", "bool", "complex_field.val_a",
		}, func(s string) string { return s })
		require.NoError(t, err)
		require.NoError(t, UpdatePillarByFieldMask(&currentPillar, &updatePillar, mask))
		require.NotNil(t, currentPillar.Int)
		require.NotNil(t, currentPillar.String)
		require.NotNil(t, currentPillar.ComplexField.ValB)
		require.Nil(t, currentPillar.Time)
		require.Nil(t, currentPillar.Bool)
		require.Nil(t, currentPillar.ComplexField.ValA)
	})
}
