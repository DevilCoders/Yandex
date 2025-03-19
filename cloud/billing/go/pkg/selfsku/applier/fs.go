package applier

import (
	"context"
	"fmt"
	"os"
	"path/filepath"

	"gopkg.in/yaml.v2"

	"a.yandex-team.ru/cloud/billing/go/pkg/selfsku/applicables"
)

var _ applicables.Applier = &FSApplier{}

type FSApplier struct {
	root string
}

func NewFSApplier(path string) (*FSApplier, error) {
	st, err := os.Stat(path)
	if err != nil {
		return nil, fmt.Errorf("can not stat path '%s': %w", path, err)
	}
	if !st.IsDir() {
		return nil, fmt.Errorf("path '%s' should be dir", path)
	}
	return &FSApplier{root: path}, nil
}

func (a *FSApplier) ApplyUnits(_ context.Context, input []applicables.Unit) error {
	return a.writeYaml("units", input)
}

func (a *FSApplier) ApplyServices(_ context.Context, input []applicables.Service) error {
	return a.writeYaml("services", input)
}

func (a *FSApplier) ApplySchemas(_ context.Context, input []applicables.Schema) error {
	return a.writeYaml("schemas", input)
}

func (a *FSApplier) ApplySkus(_ context.Context, input []applicables.Sku) error {
	return a.writeYaml("skus", input)
}

func (a *FSApplier) writeYaml(name string, input interface{}) error {
	if err := a.mkdir(name); err != nil {
		return err
	}
	f, err := os.OpenFile(a.path(name, name+".yaml"), os.O_CREATE|os.O_WRONLY, 0644)
	if err != nil {
		return fmt.Errorf("can not create %s file : %w", name, err)
	}
	enc := yaml.NewEncoder(f)
	if err := enc.Encode(input); err != nil {
		return fmt.Errorf("can not encode %s to file: %w", name, err)
	}
	if err := enc.Close(); err != nil {
		return fmt.Errorf("can not encode %s to file: %w", name, err)
	}
	if err := f.Close(); err != nil {
		return fmt.Errorf("close %s file : %w", name, err)
	}
	return nil
}

func (a *FSApplier) path(name ...string) string {
	return filepath.Join(append([]string{a.root}, name...)...)
}

func (a *FSApplier) mkdir(name string) error {
	dir := a.path(name)
	err := os.MkdirAll(dir, 0755)
	if err != nil {
		return fmt.Errorf("'%s' dir creation error: %w", dir, err)
	}
	return nil
}
