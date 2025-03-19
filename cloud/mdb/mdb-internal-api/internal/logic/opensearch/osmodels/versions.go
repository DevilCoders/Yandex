package osmodels

import (
	"encoding/json"
	"fmt"
	"regexp"
	"strconv"

	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type Version struct {
	ID             string
	Actual         string
	Deprecated     bool
	Default        bool
	UpdatableTo    []Version
	major          int
	minor          int
	patch          int
	linerizedValue int64
}

var versionRegexp *regexp.Regexp = regexp.MustCompile(`^(\d{1,2})\.(\d{1,2})\.(\d{1,2})$`)

func newVersion(id, actual string, isDeprecated, isDefault bool) (Version, error) {
	var maj, min, ptc int64
	if !versionRegexp.MatchString(actual) {
		return Version{}, xerrors.Errorf("incorrect version format %q", actual)
	}

	sl := versionRegexp.FindStringSubmatch(actual)
	maj, err := strconv.ParseInt(sl[1], 10, 32)
	if err == nil {
		min, err = strconv.ParseInt(sl[2], 10, 32)
	}
	if err == nil {
		ptc, err = strconv.ParseInt(sl[3], 10, 32)
	}
	if err != nil {
		return Version{}, xerrors.Errorf("incorrect version format %q", actual)
	}
	lv := maj*(100*100) + min*100 + ptc
	if id == "" {
		id = fmt.Sprintf("%d.%d", maj, min)
	}
	return Version{
		ID:             id,
		Actual:         actual,
		Deprecated:     isDeprecated,
		Default:        isDefault,
		major:          int(maj),
		minor:          int(min),
		patch:          int(ptc),
		linerizedValue: lv,
	}, nil
}

func mustNewVersion(id, actual string, isDeprecated, isDefault bool) Version {
	result, err := newVersion(id, actual, isDeprecated, isDefault)
	if err != nil {
		panic(err)
	}
	return result
}

func (v Version) Less(o Version) bool {
	return v.linerizedValue < o.linerizedValue
}

func (v Version) LessOrEqual(o Version) bool {
	return v.linerizedValue <= o.linerizedValue
}

func (v Version) Equal(o Version) bool {
	return v.linerizedValue == o.linerizedValue
}

func (v Version) GreaterOrEqual(o Version) bool {
	return v.linerizedValue >= o.linerizedValue
}

func (v Version) Greater(o Version) bool {
	return v.linerizedValue > o.linerizedValue
}

func (v Version) EncodedID() string {
	return fmt.Sprintf("%d%02d", v.major, v.minor)
}

type SupportedVersions []Version

func MustLoadVersions(vs []logic.Version) SupportedVersions {
	var result SupportedVersions
	for _, v := range vs {
		result = append(result, mustNewVersion(v.Name, v.ID, v.Deprecated, v.Default))
	}
	for i, v := range vs {
		for _, uv := range v.UpdatableTo {
			pv, err := result.ParseVersion(uv)
			if err != nil {
				panic(err)
			}
			result[i].UpdatableTo = append(result[i].UpdatableTo, pv)
		}
	}
	return result
}

func (sv SupportedVersions) ParseVersion(version string) (Version, error) {
	for _, ver := range sv {
		if ver.ID == version {
			return ver, nil
		}
	}
	return Version{}, semerr.InvalidInput("invalid OpenSearch version id")
}

func (sv SupportedVersions) DefaultVersion() (Version, error) {
	if len(sv) == 0 {
		return Version{}, xerrors.New("no versions available")
	}
	for _, ver := range sv {
		if ver.Default {
			return ver, nil
		}
	}
	return sv[0], nil
}

func (v *Version) MarshalJSON() ([]byte, error) {
	return json.Marshal(v.Actual)
}

func (v *Version) UnmarshalJSON(b []byte) error {
	var s string
	var err error
	if err = json.Unmarshal(b, &s); err != nil {
		return err
	}
	if s == "" {
		return xerrors.New("empty version")
	}
	*v, err = newVersion("", s, false, false)
	return err
}
