package tilde

import (
	"os"
	"path/filepath"
	"runtime"
	"strings"

	"a.yandex-team.ru/library/go/core/xerrors"
)

const (
	// LinuxHome ...
	LinuxHome = "HOME"
)

// TODO: think about replacing this custom code with https://github.com/mitchellh/go-homedir/
// There are also some comments in issues about os/user functions

// Expand provided path if it is prefixed with ~
func Expand(path string) (string, error) {
	if !strings.HasPrefix(path, "~") {
		return path, nil
	}

	home, err := Home()
	if err != nil {
		return "", err
	}

	return filepath.Join(home, path[1:]), nil
}

// Home returns current user's home directory
func Home() (string, error) {
	home := ""

	if home = os.Getenv("HOME"); home != "" {
		return home, nil
	}

	switch runtime.GOOS {
	case "windows":
		home = os.Getenv("USERPROFILE")
		if home == "" {
			home = filepath.Join(os.Getenv("HOMEDRIVE"), os.Getenv("HOMEPATH"))
		}

		if home == "" {
			return "", xerrors.New("HOME, USERPROFILE, HOMEDRIVE/HOMEPATH are blank")
		}

	default:
		if home == "" {
			return "", xerrors.New("HOME is blank")
		}
	}

	return home, nil
}
