package ssmodels

import ( //"encoding/json"
	"encoding/json"
	"reflect"
	"testing"

	"github.com/stretchr/testify/assert"
	//"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/resources"
)

func TestConfig2016sp2_ClusterConfigMerge(t *testing.T) {
	tests := []struct {
		name           string
		baseConfig     SQLServerConfig
		addConfig      SQLServerConfig
		expectedConfig SQLServerConfig
	}{
		{"Empty source doesn't change destination config",
			&ConfigBase{AuditLevel: optional.NewInt64(2)},
			&ConfigBase{},
			&ConfigBase{AuditLevel: optional.NewInt64(2)}},
		{"Change only empty field in destination config",
			&ConfigBase{AuditLevel: optional.NewInt64(2)},
			&ConfigBase{MaxDegreeOfParallelism: optional.NewInt64(30)},
			&ConfigBase{AuditLevel: optional.NewInt64(2),
				MaxDegreeOfParallelism: optional.NewInt64(30)},
		},
		{"Rewriting values in destination config with values from source config",
			&ConfigBase{AuditLevel: optional.NewInt64(2),
				MaxDegreeOfParallelism: optional.NewInt64(25)},
			&ConfigBase{MaxDegreeOfParallelism: optional.NewInt64(30)},
			&ConfigBase{AuditLevel: optional.NewInt64(2),
				MaxDegreeOfParallelism: optional.NewInt64(30)},
		},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			mergeResult, err := ClusterConfigMerge(tt.baseConfig, tt.addConfig)
			require.Truef(t, reflect.DeepEqual(mergeResult, tt.expectedConfig),
				"ClusterConfigMerge() got = %+v, want %+v", mergeResult, tt.expectedConfig)
			require.NoError(t, err)
		})
	}
}

func TestBaseConfigValidation(t *testing.T) {
	tests := []struct {
		name        string
		baseConfig  SQLServerConfig
		expectedErr string
	}{
		// Test for validation ond inherited settings
		{"AuditLevel is too high",
			&ConfigBase{AuditLevel: optional.NewInt64(10000)},
			"AuditLevel must be in range from 0 to 3 instead of: 10000"},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			err := tt.baseConfig.Validate(resources.Preset{})
			require.EqualError(t, err, tt.expectedErr)
		})
	}
}

func TestConfig2016sp2_ClusterConfigMergeByFields(t *testing.T) {
	tests := []struct {
		name           string
		baseConfig     SQLServerConfig
		addConfig      SQLServerConfig
		expectedConfig SQLServerConfig
		fields         []string
	}{
		{"Empty source doesn't change destination config with empty fields",
			&ConfigBase{AuditLevel: optional.NewInt64(2)},
			&ConfigBase{},
			&ConfigBase{AuditLevel: optional.NewInt64(2)},
			[]string{},
		},
		{"Empty source doesn't change destination config with empty fields",
			&ConfigBase{AuditLevel: optional.NewInt64(2)},
			&ConfigBase{},
			&ConfigBase{AuditLevel: optional.NewInt64(2)},
			[]string{},
		},
		{"Empty source resets destination config if fields set",
			&ConfigBase{AuditLevel: optional.NewInt64(2), MaxDegreeOfParallelism: optional.NewInt64(10)},
			&ConfigBase{},
			&ConfigBase{MaxDegreeOfParallelism: optional.NewInt64(10)},
			[]string{"AuditLevel"},
		},
		{"Change destination  if fields set",
			&ConfigBase{AuditLevel: optional.NewInt64(2), MaxDegreeOfParallelism: optional.NewInt64(10)},
			&ConfigBase{AuditLevel: optional.NewInt64(3)},
			&ConfigBase{AuditLevel: optional.NewInt64(3), MaxDegreeOfParallelism: optional.NewInt64(10)},
			[]string{"AuditLevel"},
		},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			mergeResult, err := ClusterConfigMergeByFields(tt.baseConfig, tt.addConfig, tt.fields)
			require.Truef(t, reflect.DeepEqual(mergeResult, tt.expectedConfig),
				"ClusterConfigMerge()\ngot = %+v\nwant %+v", mergeResult, tt.expectedConfig)
			require.NoError(t, err)
		})
	}
}

type A struct {
	B optional.Int64
}

func TestMarshal(t *testing.T) {
	a := A{optional.NewInt64(5)}
	b, err := optional.MarshalStructWithOnlyOptionalFields(&a)
	require.NoError(t, err)
	a1 := A{}
	err = optional.UnmarshalToStructWithOnlyOptionalFields(b, &a1)
	require.NoError(t, err)
	assert.Equal(t, int(a1.B.Int64), 5)

	conf := &ConfigBase{
		AuditLevel: optional.NewInt64(2),
	}
	b, err = json.Marshal(conf)
	require.NoError(t, err)
	conf1 := &ConfigBase{}
	err = json.Unmarshal(b, conf1)
	require.NoError(t, err)
	assert.Equal(t, int(conf1.AuditLevel.Int64), 2)
}
