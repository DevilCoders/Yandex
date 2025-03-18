package govendor

import (
	"os"

	"golang.org/x/mod/modfile"
)

func GetGoModVersion(goModPath string) (string, error) {
	goModBytes, err := os.ReadFile(goModPath)
	if err != nil {
		if os.IsNotExist(err) {
			return "", nil
		}
		return "", err
	}

	goMod, err := modfile.Parse(goModPath, goModBytes, nil)
	if err != nil {
		return "", err
	}

	if goMod.Go == nil {
		return "", nil
	}

	return goMod.Go.Version, nil
}
