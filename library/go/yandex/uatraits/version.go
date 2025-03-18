package uatraits

import (
	"strconv"
	"strings"

	"a.yandex-team.ru/library/go/core/xerrors"
)

type Version struct {
	Major int
	Minor int
	Patch int
	Build int
}

func compareIntegers(v1, v2 int) int {
	if v1 > v2 {
		return 1
	}
	if v1 < v2 {
		return -1
	}
	return 0
}

func (v Version) CompareTo(other Version) int {
	if majorCompare := compareIntegers(v.Major, other.Major); majorCompare != 0 {
		return majorCompare
	}
	if minorCompare := compareIntegers(v.Minor, other.Minor); minorCompare != 0 {
		return minorCompare
	}
	if patchCompare := compareIntegers(v.Patch, other.Patch); patchCompare != 0 {
		return patchCompare
	}
	return compareIntegers(v.Build, other.Build)
}

func parseVersion(value string) (Version, error) {
	version := Version{}

	items := strings.Split(strings.TrimSpace(value), ".")
	if len(items) > 4 {
		return version, xerrors.New("cannot parse version: value has more than four parts")
	}

	parsedIntegers := make([]int, 4)
	for i, item := range items {
		intValue, err := strconv.Atoi(item)
		if err != nil {
			return version, xerrors.Errorf("part '%s' cannot be parsed as integer", item)
		}
		if intValue < 0 {
			return version, xerrors.New("version part must be positive integer")
		}
		parsedIntegers[i] = intValue
	}

	version.Major = parsedIntegers[0]
	version.Minor = parsedIntegers[1]
	version.Patch = parsedIntegers[2]
	version.Build = parsedIntegers[3]

	return version, nil
}
