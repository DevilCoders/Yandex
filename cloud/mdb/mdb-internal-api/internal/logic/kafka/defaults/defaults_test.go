package defaults

import (
	"testing"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/kafka/helpers"
)

func TestDefaults_GetInt64ValueOrDefault(t *testing.T) {
	t.Run("When nil value then get default", func(t *testing.T) {
		var defaultValue int64 = 100
		require.Equal(t, defaultValue, GetInt64ValueOrDefault(nil, defaultValue))
	})

	t.Run("When not nil value then get value", func(t *testing.T) {
		var value = helpers.Pointer[int64](int64(177))
		var defaultValue int64 = 100
		require.Equal(t, int64(177), GetInt64ValueOrDefault(value, defaultValue))
	})
}

func TestDefaults_GetMessageMaxBytesOrDefault(t *testing.T) {
	t.Run("When nil value then get default", func(t *testing.T) {
		require.Equal(t, DefaultMessageMaxBytes, GetMessageMaxBytesOrDefault(nil))
	})

	t.Run("When not nil value then get value", func(t *testing.T) {
		var messageMaxBytes = helpers.Pointer[int64](int64(1991))
		require.Equal(t, int64(1991), GetMessageMaxBytesOrDefault(messageMaxBytes))
	})
}

func TestDefaults_GetReplicaFetchMaxBytesOrDefault(t *testing.T) {
	t.Run("When nil value then get default", func(t *testing.T) {
		require.Equal(t, DefaultReplicaFetchMaxBytes, GetReplicaFetchMaxBytesOrDefault(nil))
	})

	t.Run("When not nil value then get value", func(t *testing.T) {
		var replicaFetchMaxBytes = helpers.Pointer[int64](int64(777111999))
		require.Equal(t, int64(777111999), GetReplicaFetchMaxBytesOrDefault(replicaFetchMaxBytes))
	})
}
