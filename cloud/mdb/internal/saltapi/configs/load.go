package configs

import (
	"io/ioutil"
	"path"

	"gopkg.in/yaml.v2"

	"a.yandex-team.ru/library/go/core/xerrors"
)

// TODO: specialized error holding slice of errors

func LoadMinion() (Minion, []error) {
	var cfg Minion
	errs := loadConfigs("/etc/salt/minion.conf", "/etc/salt/minion.d", &cfg)
	return cfg, errs
}

func loadConfigs(mainPath, dirPath string, cfg interface{}) []error {
	var errs []error
	if err := loadFile(mainPath, cfg); err != nil {
		errs = append(errs, xerrors.Errorf("main path: %w", err))
	}

	files, err := ioutil.ReadDir(dirPath)
	if err != nil {
		errs = append(errs, xerrors.Errorf("read directory %q: %w", dirPath, err))
	}

	for _, f := range files {
		if f.IsDir() {
			continue
		}

		p := path.Join(dirPath, f.Name())
		if err = loadFile(p, cfg); err != nil {
			errs = append(errs, xerrors.Errorf("dir path: %w", err))
		}
	}

	return errs
}

func loadFile(p string, cfg interface{}) error {
	data, err := ioutil.ReadFile(p)
	if err != nil {
		return xerrors.Errorf("read file %q: %w", p, err)
	}

	if err = yaml.Unmarshal(data, cfg); err != nil {
		return xerrors.Errorf("unmarshal config %q: %w", p, err)
	}

	return nil
}
