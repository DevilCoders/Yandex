package chmodels

import (
	"sort"
	"strconv"
	"strings"

	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic"
)

type CHVersions struct {
	Versions []logic.Version
	Default  logic.Version
}

func VersionsFromConfig(cfg logic.CHConfig) CHVersions {
	var (
		versions       []logic.Version
		defaultVersion logic.Version
	)

	for _, v := range cfg.Versions {
		version := v
		version.ID = CutVersionToMajor(version.ID)
		version.UpdatableTo = CutVersionsToMajors(version.UpdatableTo)
		sort.Slice(version.UpdatableTo, func(i, j int) bool {
			comparison, err := VersionCompare(version.UpdatableTo[i], version.UpdatableTo[j])
			if err != nil {
				return false
			}

			return comparison > 0
		})

		versions = append(versions, version)

		if version.Default {
			defaultVersion = version
		}
	}

	sort.Slice(versions, func(i, j int) bool {
		comparison, err := VersionCompare(versions[i].ID, versions[j].ID)
		if err != nil {
			return false
		}

		return comparison > 0
	})

	if defaultVersion.ID == "" {
		defaultVersion = versions[0]
	}

	return CHVersions{
		Versions: versions,
		Default:  defaultVersion,
	}
}

// CutVersionToMajor cuts off the patch and build numbers.
//
// Example: 19.14.7.15 => 19.14
func CutVersionToMajor(version string) string {
	res := version

	parts := strings.Split(version, ".")
	if len(parts) >= 2 {
		res = strings.Join(parts[0:2], ".")
	}

	return res
}

func CutVersionsToMajors(versions []string) []string {
	majors := make([]string, 0, len(versions))
	for _, version := range versions {
		majors = append(majors, CutVersionToMajor(version))
	}
	return majors
}

func ParseVersion(version string) (int, int, error) {
	parts := strings.Split(version, ".")
	if len(parts) < 2 {
		return 0, 0, semerr.InvalidInputf("invalid version %q", version)
	}

	maj, err := strconv.Atoi(parts[0])
	if err != nil {
		return 0, 0, err
	}
	min, err := strconv.Atoi(parts[1])
	return maj, min, err
}

func VersionGreaterOrEqual(version string, major, minor int) (bool, error) {
	maj, min, err := ParseVersion(version)
	if err != nil {
		return false, err
	}

	if maj > major {
		return true, nil
	}

	return maj == major && min >= minor, nil
}

func VersionCompare(versionL, versionR string) (int64, error) {
	majorL, minorL, err := ParseVersion(versionL)
	if err != nil {
		return 0, err
	}

	majorR, minorR, err := ParseVersion(versionR)
	if err != nil {
		return 0, err
	}

	if majorL > majorR {
		return 1, nil
	} else if majorL < majorR {
		return -1, nil
	}

	if minorL > minorR {
		return 1, nil
	} else if minorL < minorR {
		return -1, nil
	}

	return 0, nil
}
