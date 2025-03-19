package osmodels

import (
	"reflect"
	"testing"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func TestDataNodeConfig_Merge(t *testing.T) {
	tests := []struct {
		name           string
		baseConfig     *DataNodeConfig
		addConfig      *DataNodeConfig
		expectedConfig DataNodeConfig
	}{
		{"Empty add doesn't change base config",
			&DataNodeConfig{FielddataCacheSize: optional.NewString("100mb")},
			&DataNodeConfig{},
			DataNodeConfig{FielddataCacheSize: optional.NewString("100mb")},
		},
		{"Change only empty field in destination config",
			&DataNodeConfig{
				FielddataCacheSize: optional.NewString("100mb"),
			},
			&DataNodeConfig{
				MaxClauseCount: optional.NewInt64(65),
			},
			DataNodeConfig{
				FielddataCacheSize: optional.NewString("100mb"),
				MaxClauseCount:     optional.NewInt64(65),
			},
		},
		{"Rewriting values in base config with values from add config",
			&DataNodeConfig{
				FielddataCacheSize:     optional.NewString("100mb"),
				MaxClauseCount:         optional.NewInt64(65),
				ReindexRemoteWhitelist: optional.NewString("*.mdb.yandexcloud.net"),
			},
			&DataNodeConfig{
				FielddataCacheSize: optional.NewString("200mb"),
			},
			DataNodeConfig{
				FielddataCacheSize:     optional.NewString("200mb"),
				MaxClauseCount:         optional.NewInt64(65),
				ReindexRemoteWhitelist: optional.NewString("*.mdb.yandexcloud.net"),
			},
		},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			mergeResult, err := tt.baseConfig.Merge(tt.addConfig)
			require.Truef(t, reflect.DeepEqual(mergeResult, tt.expectedConfig),
				"Merge() got = %+v, want %+v", mergeResult, tt.expectedConfig)
			require.NoError(t, err)
		})
	}
}

func TestValidateSizeValues(t *testing.T) {
	testsValid := []string{
		"100mb",
		"100b",
		"100g",
		"90%",
		"100KB",
		"100Pb",
		"100mB",
		"01B",
		"0B",
	}
	for _, value := range testsValid {
		t.Run("Validate valid size value", func(t *testing.T) {
			err := validateSizeValue(value, "test.setting.size")
			require.NoError(t, err)
		})
	}

	testsInvalid := []string{
		"badvalue",
		"10 kb",
		"-10kb",
		"10KiB",
		"10.1KiB",
		"101%",
		"-10%",
	}
	for _, value := range testsInvalid {
		t.Run("Validate invalid size value", func(t *testing.T) {
			err := validateSizeValue(value, "test.setting.size")
			require.Error(t, err)
		})
	}
}

func TestValidateInt64RangeValue(t *testing.T) {
	var max int64 = 1000
	var min int64 = 10

	testsValid := []int64{
		1000,
		10,
		20,
	}
	for _, value := range testsValid {
		t.Run("Validate valid range value", func(t *testing.T) {
			err := validateInt64RangeValue(value, min, max, "test.setting.size")
			require.NoError(t, err)
		})
	}

	testsInvalid := []int64{
		9,
		10000,
		-1,
	}
	for _, value := range testsInvalid {
		t.Run("Validate invalid range value", func(t *testing.T) {
			err := validateInt64RangeValue(value, min, max, "test.setting.size")
			require.Error(t, err)
		})
	}
}

func TestCombineValidationErrors(t *testing.T) {
	err := combineValidationErrors(xerrors.New("1"), xerrors.New("2"))
	require.Error(t, err, "1;2")

	err = combineValidationErrors(nil, xerrors.New("2"), nil)
	require.Error(t, err, "2")

	err = combineValidationErrors(nil, nil)
	require.NoError(t, err)
}
