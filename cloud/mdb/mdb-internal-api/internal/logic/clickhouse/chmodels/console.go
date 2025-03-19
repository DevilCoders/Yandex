package chmodels

import (
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models"
	consolemodels "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/console"
)

type ClickhouseClustersConfig struct {
	consolemodels.ClustersConfig

	MLModelName      consolemodels.NameValidator
	MLModelURI       consolemodels.NameValidator
	FormatSchemaName consolemodels.NameValidator
	FormatSchemaURI  consolemodels.NameValidator

	Versions          []string
	AvailableVersions []logic.Version
	DefaultVersion    logic.Version
}

func ExtendClustersConfig(config consolemodels.ClustersConfig, versionNames []string, allVersions []logic.Version, defaultVersion logic.Version) ClickhouseClustersConfig {
	return ClickhouseClustersConfig{
		ClustersConfig: config,
		MLModelName: consolemodels.NewNameValidator(
			models.DefaultMlModelNamePattern,
			models.DefaultMlModelNameMinLen,
			models.DefaultMlModelNameMaxLen,
		),
		MLModelURI: consolemodels.NewNameValidator(
			models.DefaultMLModelURIPattern,
			models.DefaultMLModelURIMinLen,
			models.DefaultMLModelURIMaxLen,
		),
		FormatSchemaName: consolemodels.NewNameValidator(
			models.DefaultFormatSchemaNamePattern,
			models.DefaultFormatSchemaNameMinLen,
			models.DefaultFormatSchemaNameMaxLen,
		),
		FormatSchemaURI: consolemodels.NewNameValidator(
			models.DefaultFormatSchemaURIPattern,
			models.DefaultFormatSchemaURIMinLen,
			models.DefaultFormatSchemaURIMaxLen,
		),
		Versions:          versionNames,
		AvailableVersions: allVersions,
		DefaultVersion:    defaultVersion,
	}
}
