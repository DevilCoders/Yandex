package main

import (
	"fmt"
	"os"
	"path/filepath"
	"strings"

	"github.com/spf13/cobra"
	"gopkg.in/yaml.v2"

	"a.yandex-team.ru/cloud/billing/go/pkg/selfsku/bundler"
)

var cmd = cobra.Command{
	Use:   "migrator <service> input.yaml",
	Short: "extract bundle content to filesystem",
	Args:  cobra.ExactArgs(2),
	RunE:  migrate,
}

func main() {
	if err := cmd.Execute(); err != nil {
		os.Exit(1)
	}
}

func migrate(cmd *cobra.Command, args []string) error {
	cmd.SilenceUsage = true

	path := args[1]
	bndl, srv, err := bundler.MigrateSkuFile(path, args[0])
	if err != nil {
		return err
	}

	ext := filepath.Ext(path)
	fn := strings.TrimSuffix(path, ext)

	if err := writeYaml(fmt.Sprintf("%s_skus.yaml", fn), srv); err != nil {
		return err
	}

	if err := writeYaml(fmt.Sprintf("%s_bundle.yaml", fn), bndl); err != nil {
		return err
	}
	return nil
}

func writeYaml(path string, input interface{}) error {
	f, err := os.OpenFile(path, os.O_CREATE|os.O_WRONLY, 0644)
	if err != nil {
		return fmt.Errorf("can not create %s file : %w", path, err)
	}
	enc := yaml.NewEncoder(f)
	if err := enc.Encode(input); err != nil {
		return fmt.Errorf("can not encode %s to file: %w", path, err)
	}
	if err := enc.Close(); err != nil {
		return fmt.Errorf("can not encode %s to file: %w", path, err)
	}
	if err := f.Close(); err != nil {
		return fmt.Errorf("close %s file : %w", path, err)
	}
	return nil
}
