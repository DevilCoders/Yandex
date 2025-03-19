package chmodels

import (
	"reflect"
	"testing"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
)

func userSettingsTestValidate(t *testing.T, fieldName string, args ...interface{}) {
	var value reflect.Value

	for i, arg := range args {
		if i%2 == 0 {
			value = reflect.ValueOf(arg)
			continue
		}

		valid := arg.(bool)
		var us UserSettings

		field := reflect.ValueOf(&us).Elem().FieldByName(fieldName)
		field.Set(value)
		err := us.Validate()

		if valid {
			require.NoError(t, err, "Setting %s to %s must throw no errors", fieldName, value.Interface())
		} else {
			require.Error(t, err, "Setting %s to %s must throw an error", fieldName, value.Interface())
		}
	}
}

func TestValidateUserSettings(t *testing.T) {
	// Permissions
	t.Run("ValidateReadonly", func(t *testing.T) {
		userSettingsTestValidate(t, "Readonly",
			optional.NewInt64(0), true,
			optional.NewInt64(1), true,
			optional.NewInt64(2), true,
			optional.NewInt64(3), false,
		)
	})

	// Timeouts
	t.Run("ValidateConnectTimeout", func(t *testing.T) {
		userSettingsTestValidate(t, "ConnectTimeout",
			optional.NewInt64(0), false,
			optional.NewInt64(1), true,
		)
	})
	t.Run("ValidateReceiveTimeout", func(t *testing.T) {
		userSettingsTestValidate(t, "ReceiveTimeout",
			optional.NewInt64(0), false,
			optional.NewInt64(1), true,
		)
	})
	t.Run("ValidateSendTimeout", func(t *testing.T) {
		userSettingsTestValidate(t, "SendTimeout",
			optional.NewInt64(0), false,
			optional.NewInt64(1), true,
		)
	})

	// Replication settings
	t.Run("ValidateInsertQuorumTimeout", func(t *testing.T) {
		userSettingsTestValidate(t, "InsertQuorumTimeout",
			optional.NewInt64(0), false,
			optional.NewInt64(1), true,
		)
	})
	t.Run("ValidateReplicationAlterPartitionsSync", func(t *testing.T) {
		userSettingsTestValidate(t, "ReplicationAlterPartitionsSync",
			optional.NewInt64(0), true,
			optional.NewInt64(1), true,
			optional.NewInt64(2), true,
			optional.NewInt64(3), false,
		)
	})

	// Settings of distributed queries
	t.Run("ValidateMaxReplicaDelayForDistributedQueries", func(t *testing.T) {
		userSettingsTestValidate(t, "MaxReplicaDelayForDistributedQueries",
			optional.NewInt64(0), false,
			optional.NewInt64(1), true,
		)
	})

	// I/O settings
	t.Run("ValidateMaxBlockSize", func(t *testing.T) {
		userSettingsTestValidate(t, "MaxBlockSize",
			optional.NewInt64(0), false,
			optional.NewInt64(1), true,
		)
	})
	t.Run("ValidateMaxInsertBlockSize", func(t *testing.T) {
		userSettingsTestValidate(t, "MaxInsertBlockSize",
			optional.NewInt64(0), false,
			optional.NewInt64(1), true,
		)
	})
	t.Run("ValidateMergeTreeMaxRowsToUseCache", func(t *testing.T) {
		userSettingsTestValidate(t, "MergeTreeMaxRowsToUseCache",
			optional.NewInt64(0), false,
			optional.NewInt64(1), true,
		)
	})
	t.Run("ValidateMergeTreeMaxBytesToUseCache", func(t *testing.T) {
		userSettingsTestValidate(t, "MergeTreeMaxBytesToUseCache",
			optional.NewInt64(0), false,
			optional.NewInt64(1), true,
		)
	})
	t.Run("ValidateMergeTreeMinRowsForConcurrentRead", func(t *testing.T) {
		userSettingsTestValidate(t, "MergeTreeMinRowsForConcurrentRead",
			optional.NewInt64(0), false,
			optional.NewInt64(1), true,
		)
	})
	t.Run("ValidateMergeTreeMinBytesForConcurrentRead", func(t *testing.T) {
		userSettingsTestValidate(t, "MergeTreeMinBytesForConcurrentRead",
			optional.NewInt64(0), false,
			optional.NewInt64(1), true,
		)
	})

	// Resource usage limits and query priorities
	t.Run("ValidateMaxThreads", func(t *testing.T) {
		userSettingsTestValidate(t, "MaxThreads",
			optional.NewInt64(0), false,
			optional.NewInt64(1), true,
		)
	})

	// Query complexity limits
	t.Run("ValidateMaxExecutionTime", func(t *testing.T) {
		userSettingsTestValidate(t, "MaxExecutionTime",
			optional.NewInt64(-1), false,
			optional.NewInt64(0), true,
			optional.NewInt64(1), true,
		)
	})

	// HTTP-specific settings
	t.Run("ValidateHTTPConnectionTimeout", func(t *testing.T) {
		userSettingsTestValidate(t, "HTTPConnectionTimeout",
			optional.NewInt64(0), false,
			optional.NewInt64(1), true,
		)
	})
	t.Run("ValidateHTTPReceiveTimeout", func(t *testing.T) {
		userSettingsTestValidate(t, "HTTPReceiveTimeout",
			optional.NewInt64(0), false,
			optional.NewInt64(1), true,
		)
	})
	t.Run("ValidateHTTPSendTimeout", func(t *testing.T) {
		userSettingsTestValidate(t, "HTTPSendTimeout",
			optional.NewInt64(0), false,
			optional.NewInt64(1), true,
		)
	})
	t.Run("ValidateHTTPHeadersProgressInterval", func(t *testing.T) {
		userSettingsTestValidate(t, "HTTPHeadersProgressInterval",
			optional.NewInt64(0), false,
			optional.NewInt64(50), false,
			optional.NewInt64(100), true,
			optional.NewInt64(101), true,
		)
	})
}
