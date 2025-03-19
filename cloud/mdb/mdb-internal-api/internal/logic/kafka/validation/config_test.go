package validation

import (
	"fmt"
	"testing"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/kafka/defaults"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/kafka/helpers"
)

func TestKafkaConfigValidation_IsValidSslCipherSuit(t *testing.T) {
	t.Run("When valid suit then true", func(t *testing.T) {
		suit := "TLS_AKE_WITH_AES_256_GCM_SHA384"
		require.True(t, IsValidSslCipherSuit(suit))
	})

	t.Run("When invalid suit then false", func(t *testing.T) {
		suit := "TLS_AKE_WITH_AES_256_GCM_SHqA384"
		require.False(t, IsValidSslCipherSuit(suit))
	})
}

func TestKafkaConfigValidation_IsValidSslCipherSuites(t *testing.T) {
	t.Run("When nil suites slice then no error", func(t *testing.T) {
		var suites []string = nil
		require.NoError(t, IsValidSslCipherSuitesSlice(suites))
	})

	t.Run("When valid suites slice then no error", func(t *testing.T) {
		suites := []string{
			"TLS_AKE_WITH_AES_256_GCM_SHA384",
			"TLS_DHE_RSA_WITH_AES_128_CBC_SHA256",
			"TLS_DHE_RSA_WITH_AES_128_GCM_SHA256",
			"TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256",
			"TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384",
			"TLS_RSA_WITH_AES_256_CBC_SHA",
		}
		require.NoError(t, IsValidSslCipherSuitesSlice(suites))
	})

	t.Run("When invalid suites then error", func(t *testing.T) {
		suites := []string{
			"TLS_AKE_WITH_AES_256_GCM_SHA384",
			"z",
			"TLS_DHE_RSA_WITH_AES_128_CBC_SHA256",
			"abc",
			"TLS_DHE_RSA_WITH_AES_128_GCM_SHA256",
			"TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256",
			"qqq",
			"TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384",
			"blank",
			"TLS_RSA_WITH_AES_256_CBC_SHA",
		}
		err := IsValidSslCipherSuitesSlice(suites)
		invalidSuites := []string{"abc", "blank", "qqq", "z"}
		require.Error(t, err)
		require.Equal(t, fmt.Sprintf(
			"these suites are invalid: %v. List of valid suites: [%s].",
			invalidSuites,
			"TLS_AKE_WITH_AES_128_GCM_SHA256,TLS_AKE_WITH_AES_256_GCM_SHA384,TLS_AKE_WITH_CHACHA20_POLY1305_SHA256,TLS_DHE_RSA_WITH_AES_128_CBC_SHA,TLS_DHE_RSA_WITH_AES_128_CBC_SHA256,TLS_DHE_RSA_WITH_AES_128_GCM_SHA256,TLS_DHE_RSA_WITH_AES_256_CBC_SHA,TLS_DHE_RSA_WITH_AES_256_CBC_SHA256,TLS_DHE_RSA_WITH_AES_256_GCM_SHA384,TLS_DHE_RSA_WITH_CHACHA20_POLY1305_SHA256,TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA,TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA256,TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256,TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA,TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA384,TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384,TLS_ECDHE_RSA_WITH_CHACHA20_POLY1305_SHA256,TLS_RSA_WITH_AES_128_CBC_SHA,TLS_RSA_WITH_AES_128_CBC_SHA256,TLS_RSA_WITH_AES_128_GCM_SHA256,TLS_RSA_WITH_AES_256_CBC_SHA,TLS_RSA_WITH_AES_256_CBC_SHA256,TLS_RSA_WITH_AES_256_GCM_SHA384",
		), err.Error())
	})
}

func TestKafkaConfigValidation_MessageMaxBytesMoreThenReplicaFetchMaxBytes(t *testing.T) {
	t.Run("When both nil values should return valid and default values", func(t *testing.T) {
		invalid, messageMaxBytes, replicaFetchMaxBytes := MessageMaxBytesMoreThenReplicaFetchMaxBytes(nil, nil)
		require.False(t, invalid)
		require.Equal(t, messageMaxBytes, defaults.DefaultMessageMaxBytes)
		require.Equal(t, replicaFetchMaxBytes, defaults.DefaultReplicaFetchMaxBytes)
	})

	t.Run("When only message.max.bytes is nil should return valid and default value for message.max.bytes", func(t *testing.T) {
		invalid, messageMaxBytes, replicaFetchMaxBytes := MessageMaxBytesMoreThenReplicaFetchMaxBytes(nil, helpers.Pointer[int64](int64(2000000)))
		require.False(t, invalid)
		require.Equal(t, messageMaxBytes, defaults.DefaultMessageMaxBytes)
		require.Equal(t, replicaFetchMaxBytes, int64(2000000))
	})

	t.Run("When only message.max.bytes is nil and replica.fetch.max.bytes is less then default value of message.max.bytes should return invalid", func(t *testing.T) {
		invalid, messageMaxBytes, replicaFetchMaxBytes := MessageMaxBytesMoreThenReplicaFetchMaxBytes(nil, helpers.Pointer[int64](int64(1000000)))
		require.True(t, invalid)
		require.Equal(t, messageMaxBytes, defaults.DefaultMessageMaxBytes)
		require.Equal(t, replicaFetchMaxBytes, int64(1000000))
	})

	t.Run("When only replica.fetch.max.bytes nil and message.max.bytes less then default value of replica.fetch.max.bytes should return valid", func(t *testing.T) {
		invalid, messageMaxBytes, replicaFetchMaxBytes := MessageMaxBytesMoreThenReplicaFetchMaxBytes(helpers.Pointer[int64](int64(1000000)), nil)
		require.False(t, invalid)
		require.Equal(t, messageMaxBytes, int64(1000000))
		require.Equal(t, replicaFetchMaxBytes, defaults.DefaultReplicaFetchMaxBytes)
	})

	t.Run("When only replica.fetch.max.bytes nil and message.max.bytes great then default value of replica.fetch.max.bytes should return invalid", func(t *testing.T) {
		invalid, messageMaxBytes, replicaFetchMaxBytes := MessageMaxBytesMoreThenReplicaFetchMaxBytes(
			helpers.Pointer[int64](int64(2000000)), nil)
		require.True(t, invalid)
		require.Equal(t, messageMaxBytes, int64(2000000))
		require.Equal(t, replicaFetchMaxBytes, defaults.DefaultReplicaFetchMaxBytes)
	})

	t.Run("When message.max.bytes is equal to replica.fetch.max.bytes should return valid", func(t *testing.T) {
		invalid, messageMaxBytes, replicaFetchMaxBytes := MessageMaxBytesMoreThenReplicaFetchMaxBytes(
			helpers.Pointer[int64](int64(1000)), helpers.Pointer[int64](int64(1000)))
		require.False(t, invalid)
		require.Equal(t, messageMaxBytes, int64(1000))
		require.Equal(t, replicaFetchMaxBytes, int64(1000))
	})

	t.Run("When message.max.bytes is bigger then replica.fetch.max.bytes on value less then overhead should return valid", func(t *testing.T) {
		invalid, messageMaxBytes, replicaFetchMaxBytes := MessageMaxBytesMoreThenReplicaFetchMaxBytes(
			helpers.Pointer[int64](1000+defaults.RecordLogOverheadBytes/2), helpers.Pointer[int64](int64(1000)))
		require.False(t, invalid)
		require.Equal(t, messageMaxBytes, 1000+defaults.RecordLogOverheadBytes/2)
		require.Equal(t, replicaFetchMaxBytes, int64(1000))
	})

	t.Run("When message.max.bytes is much bigger then replica.fetch.max.bytes should return invalid", func(t *testing.T) {
		invalid, messageMaxBytes, replicaFetchMaxBytes := MessageMaxBytesMoreThenReplicaFetchMaxBytes(
			helpers.Pointer[int64](int64(2000)), helpers.Pointer[int64](int64(1000)))
		require.True(t, invalid)
		require.Equal(t, messageMaxBytes, int64(2000))
		require.Equal(t, replicaFetchMaxBytes, int64(1000))
	})

	t.Run("When message.max.bytes is smaller then replica.fetch.max.bytes should return valid", func(t *testing.T) {
		invalid, messageMaxBytes, replicaFetchMaxBytes := MessageMaxBytesMoreThenReplicaFetchMaxBytes(
			helpers.Pointer[int64](1000), helpers.Pointer[int64](1000+defaults.RecordLogOverheadBytes/2))
		require.False(t, invalid)
		require.Equal(t, messageMaxBytes, int64(1000))
		require.Equal(t, replicaFetchMaxBytes, 1000+defaults.RecordLogOverheadBytes/2)
	})

	t.Run("When message.max.bytes is much smaller then replica.fetch.max.bytes should return valid", func(t *testing.T) {
		invalid, messageMaxBytes, replicaFetchMaxBytes := MessageMaxBytesMoreThenReplicaFetchMaxBytes(
			helpers.Pointer[int64](int64(1000)), helpers.Pointer[int64](int64(2000)))
		require.False(t, invalid)
		require.Equal(t, messageMaxBytes, int64(1000))
		require.Equal(t, replicaFetchMaxBytes, int64(2000))
	})
}
