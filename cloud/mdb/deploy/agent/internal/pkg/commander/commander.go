package commander

import (
	"context"
	"path/filepath"
	"strings"
	"time"

	"a.yandex-team.ru/cloud/mdb/internal/semerr"
)

const ImagePrefix = "image-"

type DataSource struct {
	Version string
	URI     string
}

func VersionFromFromName(name string) string {
	// Remove ext from key: image-1644272755-r9117567.txz
	version := strings.TrimSuffix(name, filepath.Ext(name))
	return strings.TrimPrefix(version, ImagePrefix)
}

type Options struct {
	Colored  bool
	Progress bool
}

type Command struct {
	ID      string
	Name    string
	Args    []string
	Source  DataSource
	Options Options
}

func (c Command) Valid() error {
	if c.ID == "" {
		return semerr.InvalidInput("emtpy command Id")
	}
	if c.Name == "" {
		return semerr.InvalidInput("empty command Name")
	}
	return nil
}

type Result struct {
	CommandID  string
	ExitCode   int
	Error      error
	Stdout     string
	Stderr     string
	StartedAt  time.Time
	FinishedAt time.Time
}

//go:generate ../../../../../scripts/mockgen.sh CommandSourcer,ProgressTrackedCommandSourcer

// CommandSourcer provides API for pending commands and notification on execution statuses
type CommandSourcer interface {
	Commands(ctx context.Context) <-chan Command
	Done(ctx context.Context, result Result)
}

type Progress struct {
	CommandID string
	Message   string
}

type ProgressTracker interface {
	Track(ctx context.Context, progress Progress)
}

type ProgressTrackedCommandSourcer interface {
	CommandSourcer
	ProgressTracker
}
