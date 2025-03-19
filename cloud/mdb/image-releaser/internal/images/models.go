package images

import (
	"strings"

	"a.yandex-team.ru/cloud/mdb/internal/compute/compute"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type OS int

const (
	OSUnknown OS = iota
	OSLinux
	OSWindows
)

func (o OS) ComputeOS() compute.OS {
	switch o {
	case OSWindows:
		return compute.OSWindows
	default:
		return compute.OSLinux
	}
}

func ParseOS(unparsed string) (OS, error) {
	filtered := strings.ToLower(strings.TrimSpace(unparsed))
	switch filtered {
	case "linux":
		return OSLinux, nil
	case "windows":
		return OSWindows, nil
	}
	return OSUnknown, xerrors.Errorf("parse OS: %q", unparsed)
}
