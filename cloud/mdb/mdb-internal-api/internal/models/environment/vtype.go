package environment

import "a.yandex-team.ru/library/go/core/xerrors"

type VType string

const (
	VTypeCompute VType = "compute"
	VTypePorto   VType = "porto"
	VTypeAWS     VType = "aws"
)

var vTypeMapping = map[VType]struct{}{
	VTypeCompute: struct{}{},
	VTypePorto:   struct{}{},
	VTypeAWS:     struct{}{},
}

func ParseVType(str string) (VType, error) {
	vt := VType(str)
	if _, ok := vTypeMapping[vt]; !ok {
		return "", xerrors.Errorf("invalid vtype: %s", str)
	}

	return vt, nil
}
