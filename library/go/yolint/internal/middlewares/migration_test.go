package middlewares

import (
	"testing"

	"github.com/stretchr/testify/require"
	"golang.org/x/tools/go/analysis/analysistest"

	"a.yandex-team.ru/library/go/test/yatest"
	"a.yandex-team.ru/library/go/yolint/internal/passes/errcheck"
)

func TestMigration(t *testing.T) {
	testdata := analysistest.TestData()

	// TODO(prime@): provide helpers accepting relative path.
	migrationConfigPath = yatest.SourcePath("library/go/yolint/internal/middlewares/testdata/src/migration/config.yaml")

	analysistest.Run(t, testdata, Migration(errcheck.Analyzer), "migration/...")
}

func TestMigrationConfig(t *testing.T) {
	inputs := []struct {
		Name      string
		Config    migrationConfig
		Whitelist MigrationWhitelist
	}{
		{
			Name:      "Empty",
			Whitelist: MigrationWhitelist{},
		},
		{
			Name: "SameCheckInMultipleMigrations",
			Config: migrationConfig{
				Migrations: map[string]migrationRule{
					"check": {
						Packages: []string{"foo"},
					},
					"another": {
						Checks:   []string{"check"},
						Packages: []string{"bar"},
					},
				},
			},
			Whitelist: MigrationWhitelist{
				"check": map[string]struct{}{
					"foo": {},
					"bar": {},
				},
			},
		},
	}

	for _, input := range inputs {
		t.Run(input.Name, func(t *testing.T) {
			require.Equal(t, input.Whitelist, input.Config.whitelist())
		})
	}
}
