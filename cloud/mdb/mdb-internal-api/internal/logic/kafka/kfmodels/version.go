package kfmodels

import (
	"fmt"
	"strconv"
	"strings"

	"golang.org/x/exp/slices"

	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/library/go/core/xerrors"
)

const (
	Version2_1      = "2.1"
	Version2_6      = "2.6"
	Version2_8      = "2.8"
	Version3_0      = "3.0"
	Version3_1      = "3.1"
	Version3_2      = "3.2"
	СonfigVersion3  = "3"
	DefaultVersion  = Version3_0
	DefaultVersion3 = Version3_0
)

var (
	AllValidKafkaVersions3x = []string{
		Version3_0,
		Version3_1,
		Version3_2,
	}
)

type KafkaVersion struct {
	Name  string
	Major int
	Minor int

	Deprecated          bool
	VisibleInConsoleMDB bool
	VisibleInConsoleDC  bool
	PackageVersion      string
	UpdatableTo         []string
}

var Versions = []*KafkaVersion{
	{
		Name:                Version3_2,
		Deprecated:          false,
		VisibleInConsoleMDB: true,
		VisibleInConsoleDC:  false,
		PackageVersion:      "3.2.0-java11",
		UpdatableTo:         []string{},
	},
	{
		Name:                Version3_1,
		Deprecated:          false,
		VisibleInConsoleMDB: true,
		VisibleInConsoleDC:  true,
		PackageVersion:      "3.1.1-java11",
		UpdatableTo:         []string{},
	},
	{
		Name:                Version3_0,
		Deprecated:          false,
		VisibleInConsoleMDB: true,
		VisibleInConsoleDC:  true,
		PackageVersion:      "3.0.1-java11",
		UpdatableTo:         []string{Version3_1},
	},
	{
		Name:                Version2_8,
		Deprecated:          false,
		VisibleInConsoleMDB: true,
		VisibleInConsoleDC:  true,
		PackageVersion:      "2.8.1.1-java11",
		UpdatableTo:         []string{Version3_0, Version3_1},
	},
	{
		Name:                Version2_6,
		Deprecated:          true,
		VisibleInConsoleMDB: true,
		VisibleInConsoleDC:  false,
		PackageVersion:      "2.6.0-java11",
		UpdatableTo:         []string{Version2_8, Version3_0, Version3_1},
	},
	{
		Name:                Version2_1,
		Deprecated:          true,
		VisibleInConsoleMDB: true,
		VisibleInConsoleDC:  false,
		PackageVersion:      "2.1.1-java11",
		UpdatableTo:         []string{Version2_6, Version2_8, Version3_0, Version3_1},
	},
}

var VersionsVisibleInConsoleMDB []*KafkaVersion
var NamesOfVersionsVisibleInConsoleMDB []string
var VersionsVisibleInConsoleDC []*KafkaVersion

func init() {
	for _, version := range Versions {
		components := strings.Split(version.Name, ".")

		var err error
		version.Major, err = strconv.Atoi(components[0])
		if err != nil {
			panic(fmt.Sprintf("failed to parse Kafka version %q: %s", version.Name, err))
		}
		version.Minor, err = strconv.Atoi(components[1])
		if err != nil {
			panic(fmt.Sprintf("failed to parse Kafka version %q: %s", version.Name, err))
		}

		if version.VisibleInConsoleMDB {
			VersionsVisibleInConsoleMDB = append(VersionsVisibleInConsoleMDB, version)
			NamesOfVersionsVisibleInConsoleMDB = append(NamesOfVersionsVisibleInConsoleMDB, version.Name)
		}
		if version.VisibleInConsoleDC {
			VersionsVisibleInConsoleDC = append(VersionsVisibleInConsoleDC, version)
		}
	}
}

func FindVersion(version string) (KafkaVersion, error) {
	for _, ver := range Versions {
		if ver.Name == version {
			return *ver, nil
		}
	}
	return KafkaVersion{}, semerr.InvalidInput("unknown Apache Kafka version")
}

func ValidateVersion(version string) error {
	_, err := FindVersion(version)
	return err
}

func IsSupportedVersion3x(version string) bool {
	return slices.Contains[string](AllValidKafkaVersions3x, version)
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
