package esmodels

import "a.yandex-team.ru/cloud/mdb/internal/semerr"

var allowedPlugins7 = []string{

	// official ElasticSearch plugins
	"analysis-icu",
	"analysis-kuromoji",
	"analysis-nori",
	"analysis-phonetic",
	"analysis-smartcn",
	"analysis-stempel",
	"analysis-ukrainian",
	"ingest-attachment",
	"mapper-annotated-text",
	"mapper-murmur3",
	"mapper-size",
	"repository-azure",
	"repository-gcs",
	"repository-hdfs",
	"repository-s3",
	"transport-nio",

	// disable discovery, we managed discovery on our own
	// "discovery-azure-classic": {},
	// "discovery-ec2":           {},
	// "discovery-gce":           {},

	// disable samba store
	// "store-smb",
}

var setAllowedPlugins7 = make(map[string]struct{})

func init() {
	for _, k := range allowedPlugins7 {
		setAllowedPlugins7[k] = struct{}{}
	}
}

func ValidatePlugins(plugins []string) error {
	for _, p := range plugins {
		if _, ok := setAllowedPlugins7[p]; !ok {
			return semerr.InvalidInputf("unknown ElasticSearch plugin %q.", p)
		}
	}

	return nil
}

func AllowedPlugins(v Version) []string {
	// 8.0+ removes repository plugins
	if v.major < 8 {
		return allowedPlugins7
	}
	return []string{}
}
