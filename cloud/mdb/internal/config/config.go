package config

import (
	"context"
	"fmt"
	"io/ioutil"
	"path"

	"github.com/heetch/confita"
	"github.com/heetch/confita/backend/file"
	"gopkg.in/yaml.v2"

	"a.yandex-team.ru/cloud/mdb/internal/flags"
	"a.yandex-team.ru/library/go/core/xerrors"
)

// JoinWithConfigPath ...
func JoinWithConfigPath(filename string) string {
	changed, configPathValue := flags.ConfigPathFlag()
	if changed {
		filename = path.Join(configPathValue, filename)
	}
	return filename
}

// Load config from config or config-path if provided or from absolute path if not
func Load(filename string, cfg interface{}) error {
	_, configPath := PathByFlags(filename)
	return LoadFromAbsolutePath(configPath, cfg)
}

// LoadFromConfigPath ...
func LoadFromConfigPath(filename string, cfg interface{}) error {
	changed, configPathValue := flags.ConfigPathFlag()
	if !changed {
		return xerrors.New("config path not registered")
	}

	filename = path.Join(configPathValue, filename)
	return LoadFromAbsolutePath(filename, cfg)
}

// LoadFromAbsolutePath ...
func LoadFromAbsolutePath(filename string, cfg interface{}) error {
	loader := confita.NewLoader(file.NewBackend(filename))
	return loader.Load(context.Background(), cfg)
}

func GenerateConfigOnFlag(filename string, cfg interface{}) (bool, error) {
	changed, genPath := flags.GenConfigFlag()
	if !changed {
		return false, nil
	}

	fmt.Printf("Generating config %q to path %q\n", filename, genPath)

	data, err := yaml.Marshal(cfg)
	if err != nil {
		return false, xerrors.Errorf("generate default config: %w", err)
	}

	if err = ioutil.WriteFile(path.Join(genPath, filename), data, 0644); err != nil {
		return false, xerrors.Errorf("write default config to %q: %w", genPath, err)
	}

	fmt.Printf("Generated config %q to path %q\n", filename, genPath)
	return true, nil
}

func PathByFlags(defFilename string) (bool, string) {
	filename := defFilename
	changed, filepath := flags.ConfigFlag()
	if changed {
		return true, filepath
	}
	changed, configPathValue := flags.ConfigPathFlag()
	if changed {
		filename = path.Join(configPathValue, defFilename)
	}
	return changed, filename
}
