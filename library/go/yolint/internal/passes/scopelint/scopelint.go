package scopelint

import (
	"fmt"
	"io/ioutil"
	"strings"

	"golang.org/x/tools/go/analysis"
	"gopkg.in/yaml.v2"

	"a.yandex-team.ru/library/go/core/xerrors"
)

const (
	Name = "scopelint"
	Doc  = "scopelint enables additional linters for specific packages"
)

var (
	flagConfigPath string

	supportedLinters = make(map[string]*analysis.Analyzer)
)

func init() {
	Analyzer.Flags.StringVar(&flagConfigPath, "config", "", "path to rules config")
}

// Register marks analyzer as available to scopelint
func Register(linters ...*analysis.Analyzer) {
	for _, linter := range linters {
		supportedLinters[linter.Name] = linter
		// adopt all requirements and facts of registered linters
		Analyzer.Requires = append(Analyzer.Requires, linter.Requires...)
		Analyzer.FactTypes = append(Analyzer.FactTypes, linter.FactTypes...)
	}
}

var Analyzer = &analysis.Analyzer{
	Name:             Name,
	Doc:              Doc,
	Run:              run,
	RunDespiteErrors: true,
}

// config is a representation of YAML file with rules
// Example:
//
//     scopes:
//         a.yandex-team.ru/library/go:
//             - errcheck
//             - protonaming
type config struct {
	Scopes map[string][]string `yaml:"scopes"`
}

func run(pass *analysis.Pass) (interface{}, error) {
	lintScopes, err := loadScopes(flagConfigPath)
	if err != nil {
		return nil, fmt.Errorf("cannot load linting scopes: %w", err)
	}

	packageName := pass.Pkg.Path()

	// collect applicable linters
	applicableLinters := make(map[string]*analysis.Analyzer)
	for lintScope, linters := range lintScopes {
		if !strings.HasPrefix(packageName, lintScope) {
			continue
		}

		for _, linterName := range linters {
			if linter, ok := supportedLinters[linterName]; ok {
				applicableLinters[linterName] = linter
			}
		}
	}

	// run linters
	for _, linter := range applicableLinters {
		if _, err := linter.Run(pass); err != nil {
			return nil, err
		}
	}

	return nil, nil
}

func loadScopes(path string) (map[string][]string, error) {
	var cfg config

	if path != "" {
		buf, err := ioutil.ReadFile(path)
		if err != nil {
			return nil, fmt.Errorf("error reading scopelint config: %w", err)
		}

		if err := yaml.Unmarshal(buf, &cfg); err != nil {
			return nil, xerrors.Errorf("error unmarshalling scopelint config: %w", err)
		}
	}

	return cfg.Scopes, nil
}
