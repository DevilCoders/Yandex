package esmodels

import (
	"encoding/json"
	"fmt"
	"strings"

	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type Edition int

const (
	EditionUnknown Edition = iota
	EditionOss
	EditionBasic
	EditionGold
	EditionPlatinum
	EditionEnterprise
	EditionTrial
)

const DefaultEdition = EditionBasic
const DeprecatedEdition = EditionOss

var DefaultAvailableEditions = []Edition{
	EditionBasic,
	EditionGold,
	EditionPlatinum,
}

var (
	mapEditionToString = map[Edition]string{
		EditionUnknown:    "UNKNOWN",
		EditionOss:        "oss",
		EditionBasic:      "basic",
		EditionGold:       "gold",
		EditionPlatinum:   "platinum",
		EditionEnterprise: "enterprise",
		EditionTrial:      "trial",
	}
	nameToStatusMapping = make(map[string]Edition, len(mapEditionToString))
)

func init() {
	for status, str := range mapEditionToString {
		nameToStatusMapping[strings.ToLower(str)] = status
	}
}

func (s Edition) String() string {
	str, ok := mapEditionToString[s]
	if !ok {
		return fmt.Sprintf("UNKNOWN_ELASTICSEARCH_EDITION_%d", s)
	}

	return str
}

func (s Edition) Includes(o Edition) bool {
	return s >= o
}

var editionNotAllowedError = semerr.FailedPreconditionf("edition not allowed")

func CheckAllowedEdition(candidate Edition, allowed []Edition) error {
	for _, ed := range allowed {
		if ed == candidate {
			return nil
		}
	}

	return editionNotAllowedError
}

func ParseEdition(str string) (Edition, error) {
	s, ok := nameToStatusMapping[strings.ToLower(str)]
	if !ok {
		return EditionUnknown, xerrors.Errorf("unknown cluster edition %q", str)
	}

	return s, nil
}

func (s *Edition) MarshalJSON() ([]byte, error) {
	return json.Marshal(s.String())
}

func (s *Edition) UnmarshalJSON(b []byte) (err error) {
	var r string
	if err := json.Unmarshal(b, &r); err != nil {
		return err
	}
	if r == "" {
		*s = DeprecatedEdition
		return nil
	}
	if *s, err = ParseEdition(r); err != nil {
		return err
	}
	return nil
}

func MustLoadAllowedEditions(editions []string) []Edition {
	res, err := loadAllowedEditions(editions)
	if err != nil {
		panic(err)
	}
	return res
}

func loadAllowedEditions(editions []string) ([]Edition, error) {
	if len(editions) == 0 {
		return DefaultAvailableEditions, nil
	}

	result := make([]Edition, len(editions))
	for i, name := range editions {
		ed, err := ParseEdition(name)
		if err != nil {
			return nil, err
		}
		result[i] = ed
	}
	return result, nil
}
