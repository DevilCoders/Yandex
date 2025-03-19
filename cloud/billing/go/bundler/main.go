package main

import (
	"fmt"
	"log"
	"os"
	"path/filepath"

	"github.com/spf13/cobra"
	"gopkg.in/yaml.v2"

	"a.yandex-team.ru/cloud/billing/go/pkg/selfsku/bundler"
)

var cmd = &cobra.Command{
	Use:   fmt.Sprintf("%s [-s source_root] output_dir bundle_name", os.Args[0]),
	Short: "Run bundler export utility",
	RunE:  run,
	Args:  cobra.ExactArgs(2),
}

var sourcePath string

func init() {
	cmd.PersistentFlags().StringVarP(&sourcePath, "source", "s", "", "Path to the bundles directory")
	_ = cmd.MarkPersistentFlagRequired("source")
}

func main() {
	log.SetOutput(os.Stderr)

	if err := cmd.Execute(); err != nil {
		os.Exit(1)
	}
}

func writeYaml(dir, name string, input interface{}) error {
	path := filepath.Join(dir, name+".yaml")
	f, err := os.OpenFile(path, os.O_CREATE|os.O_WRONLY, 0644)
	if err != nil {
		return fmt.Errorf("can not create file %s: %w", path, err)
	}
	enc := yaml.NewEncoder(f)
	if err := enc.Encode(input); err != nil {
		return fmt.Errorf("can not encode to file %s: %w", path, err)
	}
	if err := enc.Close(); err != nil {
		return fmt.Errorf("can not encode to file %s: %w", path, err)
	}
	if err := f.Close(); err != nil {
		return fmt.Errorf("close file %s: %w", path, err)
	}
	return nil
}

func run(cmd *cobra.Command, args []string) error {
	destinationPath := args[0]
	bundleName := args[1]

	dataDir := os.DirFS(sourcePath)
	b, err := bundler.Load(dataDir)
	if err != nil {
		return fmt.Errorf("can not load bundle %s: %w", bundleName, err)
	}

	b = b.FilterForEnv(bundleName)
	if err = writeYaml(destinationPath, "units", b.Units()); err != nil {
		return err
	}
	if err = writeYaml(destinationPath, "services", b.Services()); err != nil {
		return err
	}
	if err = writeYaml(destinationPath, "schemas", b.Schemas()); err != nil {
		return err
	}
	if err = writeYaml(destinationPath, "skus", b.Sku(bundleName)); err != nil {
		return err
	}

	return nil
}
