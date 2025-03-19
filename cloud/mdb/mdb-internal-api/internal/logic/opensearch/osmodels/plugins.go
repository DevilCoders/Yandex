package osmodels

import "a.yandex-team.ru/cloud/mdb/internal/semerr"

var allowedPlugins = map[string]struct{}{

	// official OpenSearch plugins
	"analysis-icu":          {},
	"analysis-kuromoji":     {},
	"analysis-nori":         {},
	"analysis-phonetic":     {},
	"analysis-smartcn":      {},
	"analysis-stempel":      {},
	"analysis-ukrainian":    {},
	"ingest-attachment":     {},
	"mapper-annotated-text": {},
	"mapper-murmur3":        {},
	"mapper-size":           {},
	"repository-azure":      {},
	"repository-gcs":        {},
	"repository-hdfs":       {},
	"repository-s3":         {},
	"transport-nio":         {},

	// disable discovery, we managed discovery on our own
	// "discovery-azure-classic": {},
	// "discovery-ec2":           {},
	// "discovery-gce":           {},

	// disable samba store
	// "store-smb":             {},
}

func ValidatePlugins(plugins []string) error {
	for _, p := range plugins {
		if _, ok := allowedPlugins[p]; !ok {
			return semerr.InvalidInputf("unknown OpenSearch plugin %q.", p)
		}
	}

	return nil
}
