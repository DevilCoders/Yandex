package middlewares

import (
	"io/ioutil"
	"reflect"

	"golang.org/x/tools/go/analysis"
	"golang.org/x/xerrors"
	yaml "gopkg.in/yaml.v2"
)

const (
	migrationName = "migration"
)

type (
	migrationRule struct {
		Checks   []string `yaml:"checks"`
		Packages []string `yaml:"packages"`
	}

	migrationConfig struct {
		Migrations map[string]migrationRule `yaml:"migrations"`
	}

	// Analyzer Name -> Package
	MigrationWhitelist map[string]map[string]struct{}
)

var (
	migrationConfigPath string

	MigrationAnalyzer = &analysis.Analyzer{
		Name:       migrationName,
		Doc:        `migration loads migration config for use by other analyzers`,
		Run:        loadConfig,
		ResultType: reflect.TypeOf(MigrationWhitelist{}),
	}
)

func init() {
	MigrationAnalyzer.Flags.StringVar(&migrationConfigPath, "config", "", "path to migration config")
}

func (c *migrationConfig) whitelist() MigrationWhitelist {
	l := MigrationWhitelist{}

	for name, rule := range c.Migrations {
		checks := rule.Checks
		if len(checks) == 0 {
			checks = []string{name}
		}

		for _, check := range checks {
			// Same check can be in multiple migrations
			pkgs, ok := l[check]
			if !ok {
				pkgs = make(map[string]struct{})
				l[check] = pkgs
			}

			for _, pkg := range rule.Packages {
				pkgs[pkg] = struct{}{}
			}
		}
	}

	return l
}

func loadConfig(pass *analysis.Pass) (interface{}, error) {
	var config migrationConfig

	if migrationConfigPath != "" {
		buf, err := ioutil.ReadFile(migrationConfigPath)
		if err != nil {
			return nil, xerrors.Errorf("error loading migration config: %w", err)
		}

		if err := yaml.Unmarshal(buf, &config); err != nil {
			return nil, xerrors.Errorf("error loading migration config: %w", err)
		}
	}

	return config.whitelist(), nil
}

// Migration disables certain linters for certain packages.
func Migration(analyzer *analysis.Analyzer) *analysis.Analyzer {
	migratingAnalyzer := *analyzer

	migratingAnalyzer.Requires = append([]*analysis.Analyzer{MigrationAnalyzer}, analyzer.Requires...)

	migratingAnalyzer.Run = func(pass *analysis.Pass) (interface{}, error) {
		whitelist := pass.ResultOf[MigrationAnalyzer].(MigrationWhitelist)

		pkgs, ok := whitelist[migratingAnalyzer.Name]
		if !ok {
			return analyzer.Run(pass)
		}

		_, ok = pkgs[pass.Pkg.Path()]
		if !ok {
			return analyzer.Run(pass)
		}

		localPass := *pass
		// Ignore all diagnostics from this analysis.
		localPass.Report = func(d analysis.Diagnostic) {}

		return analyzer.Run(&localPass)
	}

	return &migratingAnalyzer
}
