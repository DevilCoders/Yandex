package validation

import (
	"sort"

	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/kafka/defaults"
)

func ValidSslCipherSuitesToMap() map[string]bool {
	result := map[string]bool{}
	for _, suit := range defaults.AllValidSslCipherSuitesSortedSlice {
		result[suit] = true
	}
	return result
}

var (
	isValidSslCipherSuite = ValidSslCipherSuitesToMap()
)

func IsValidSslCipherSuit(sslCipherSuit string) bool {
	_, ok := isValidSslCipherSuite[sslCipherSuit]
	return ok
}

func IsValidSslCipherSuitesSlice(sslCipherSuites []string) error {
	var invalidSuites []string
	for _, suit := range sslCipherSuites {
		if !IsValidSslCipherSuit(suit) {
			invalidSuites = append(invalidSuites, suit)
		}
	}
	if invalidSuites != nil {
		sort.Strings(invalidSuites)
		return semerr.InvalidInputf(
			"these suites are invalid: %v. List of valid suites: [%s].",
			invalidSuites,
			defaults.AllValidSslCipherSuitesSortedString,
		)
	}
	return nil
}

func MessageMaxBytesMoreThenReplicaFetchMaxBytes(messageMB *int64, replicaFetchMB *int64) (bool, int64, int64) {
	invalid := false
	messageMaxBytes := defaults.GetMessageMaxBytesOrDefault(messageMB)
	replicaFetchMaxBytes := defaults.GetReplicaFetchMaxBytesOrDefault(replicaFetchMB)
	if messageMB != nil || replicaFetchMB != nil {
		// Kafka use "message.max.bytes" property with record log overhead which includes
		// OFFSET_OFFSET (0 bytes) + OFFSET_LENGTH (8 bytes) + SIZE_LENGTH (4 bytes)
		// So we should compare message.max.bytes without record log overhead (12 bytes)
		invalid = messageMaxBytes-defaults.RecordLogOverheadBytes > replicaFetchMaxBytes
	}
	return invalid, messageMaxBytes, replicaFetchMaxBytes
}
