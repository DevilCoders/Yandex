package pgmigrate

import (
	"bytes"
	"encoding/json"
	"fmt"
	"os"
	"os/exec"
	"sort"
)

type Migration struct {
	Version     int     `json:"version"`
	InstalledOn *string `json:"installed_on"`
	Description string  `json:"description"`
}

type Cfg struct {
	BaseDir string
	Conn    string
}

func pgmigrate(cfg Cfg, cmd ...string) *exec.Cmd {
	var args []string
	if cfg.BaseDir != "" {
		args = append(args, fmt.Sprintf("--base_dir=%s", cfg.BaseDir))
	}
	if cfg.Conn != "" {
		args = append(args, fmt.Sprintf("--conn=%s", cfg.Conn))
	}
	return exec.Command("pgmigrate", append(args, cmd...)...)
}

func Info(cfg Cfg) ([]Migration, error) {
	cmd := pgmigrate(cfg, "info")
	out := &bytes.Buffer{}
	cmd.Stdout = out
	cmd.Stderr = os.Stderr
	if err := cmd.Run(); err != nil {
		return nil, fmt.Errorf("running %s: %w", cmd.String(), err)
	}
	migrations, err := parseInfoOut(out)
	if err != nil {
		return nil, err
	}
	return migrations, nil
}

func parseInfoOut(out *bytes.Buffer) ([]Migration, error) {
	var migrationsDict map[string]Migration
	if err := json.Unmarshal(out.Bytes(), &migrationsDict); err != nil {
		return nil, fmt.Errorf("parse pmigrate out(%s): %w", out.String(), err)
	}
	migrations := make([]Migration, 0, len(migrationsDict))
	for _, m := range migrationsDict {
		migrations = append(migrations, m)
	}
	sort.Slice(migrations, func(i, j int) bool {
		return migrations[i].Version < migrations[j].Version
	})
	return migrations, nil
}

func MigrateTo(cfg Cfg, version int) error {
	cmd := pgmigrate(cfg, "migrate", fmt.Sprintf("--target=%d", version), "--verbose")
	cmd.Stdout = os.Stdout
	cmd.Stderr = os.Stderr
	if err := cmd.Run(); err != nil {
		return fmt.Errorf("running %s: %w", cmd.String(), err)
	}
	return nil
}
