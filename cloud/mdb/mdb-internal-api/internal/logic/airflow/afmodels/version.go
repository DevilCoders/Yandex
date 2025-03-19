package afmodels

import (
	"fmt"
	"strconv"
	"strings"

	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/library/go/core/xerrors"
)

const (
	Version2_2_4   = "2.2.4"
	DefaultVersion = Version2_2_4
)

type AirflowVersion struct {
	Name  string
	Major int
	Minor int
	Patch int

	Deprecated       bool
	VisibleInConsole bool
	PackageVersion   string
	UpdatableTo      []string
}

var Versions = []*AirflowVersion{
	{
		Name:             Version2_2_4,
		Deprecated:       false,
		VisibleInConsole: true,
		PackageVersion:   "2.2.4",
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
			panic(fmt.Sprintf("failed to parse Airflow version %q: %s", version.Name, err))
		}
		version.Minor, err = strconv.Atoi(components[1])
		if err != nil {
			panic(fmt.Sprintf("failed to parse Airflow version %q: %s", version.Name, err))
		}
		version.Patch, err = strconv.Atoi(components[2])
		if err != nil {
			panic(fmt.Sprintf("failed to parse Airflow version %q: %s", version.Name, err))
		}

		if version.VisibleInConsole {
			VersionsVisibleInConsole = append(VersionsVisibleInConsole, version.Name)
		}
	}
}

func FindVersion(version string) (AirflowVersion, error) {
	for _, ver := range Versions {
		if ver.Name == version {
			return *ver, nil
		}
	}
	return AirflowVersion{}, semerr.InvalidInput("Unsupported Airflow version")
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

	if currentVersion.Major < targetVersion.Major || currentVersion.Minor < targetVersion.Minor || currentVersion.Patch < targetVersion.Patch {
		return nil
	}
	return semerr.InvalidInput("downgrade is not allowed")
}
