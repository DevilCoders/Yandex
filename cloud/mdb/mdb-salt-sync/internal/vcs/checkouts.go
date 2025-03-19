package vcs

import (
	"io/ioutil"
	"os"
	"path"
	"strings"
	"time"

	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func InitCheckouts(checkoutsPath string, checkoutMaxAge time.Duration, l log.Logger) error {
	if err := os.MkdirAll(checkoutsPath, os.ModePerm); err != nil {
		return xerrors.Errorf("failed to create checkouts dir: %w", err)
	}
	checkouts, err := ioutil.ReadDir(checkoutsPath)
	if err != nil {
		return xerrors.Errorf("failed to list dirs: %w", err)
	}
	for _, c := range checkouts {
		if !c.IsDir() {
			continue
		}
		age := time.Since(c.ModTime())
		if age > checkoutMaxAge {
			l.Infof("Remove %s checkout cause it has %s age", c.Name(), age)
			if err := removeEntirePath(path.Join(checkoutsPath, c.Name()), l); err != nil {
				return xerrors.Errorf("failed to remove %q checkout: %w", c.Name(), err)
			}
		}
	}
	return nil
}

func RemoveCheckouts(checkoutsPath string, l log.Logger) error {
	return removeEntirePath(checkoutsPath, l)
}

func pathToCheckout(checkoutsPath, repoURL, version string) string {
	return path.Join(
		checkoutsPath,
		strings.ReplaceAll(repoURL, "/", "-"),
		version,
	)
}
