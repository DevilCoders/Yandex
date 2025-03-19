package models

import (
	"fmt"
	"strconv"
	"strings"

	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/library/go/core/xerrors"
)

const (
	Version3_0_0   = "3.0.0"
	DefaultVersion = Version3_0_0
)

type MetastoreVersion struct {
	Name  string
	Major int
	Minor int

	Deprecated       bool
	VisibleInConsole bool
	PackageVersion   string
	UpdatableTo      []string
}

var Versions = []*MetastoreVersion{
	{
		Name:             Version3_0_0,
		Deprecated:       false,
		VisibleInConsole: true,
		PackageVersion:   "3.0.0",
		UpdatableTo:      []string{},
	},
}

var VersionsVisibleInConsole []string

func init() {
	for _, version := range Versions {
		components := strings.Split(version.Name, ".")

		var err error
		version.Major, err = strconv.Atoi(components[0])
		if err != nil {
			panic(fmt.Sprintf("failed to parse Metastore version %q: %s", version.Name, err))
		}
		version.Minor, err = strconv.Atoi(components[1])
		if err != nil {
			panic(fmt.Sprintf("failed to parse Metastore version %q: %s", version.Name, err))
		}

		if version.VisibleInConsole {
			VersionsVisibleInConsole = append(VersionsVisibleInConsole, version.Name)
		}
	}
}

func FindVersion(version string) (MetastoreVersion, error) {
	for _, ver := range Versions {
		if ver.Name == version {
			return *ver, nil
		}
	}
	return MetastoreVersion{}, semerr.InvalidInput("unknown Metastore version")
}

func ValidateVersion(version string) error {
	_, err := FindVersion(version)
	return err
}

func ValidateUpgrade(current string, target string) error {
	currentVersion, err := FindVersion(current)
	if err != nil {
		return xerrors.Errorf("cluster's current version is unknown: %w", err)
	}

	targetVersion, err := FindVersion(target)
	if err != nil {
		return err
	}

	if currentVersion.Major < targetVersion.Major || currentVersion.Minor < targetVersion.Minor {
		return nil
	}
	return semerr.InvalidInput("downgrade is not allowed")
}
