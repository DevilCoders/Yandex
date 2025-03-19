package environment

import (
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
)

type CloudType string

const (
	CloudTypeAWS    CloudType = "aws"
	CloudTypeYandex CloudType = "yandex"
)

var cloudTypeMapping = map[CloudType]struct{}{
	CloudTypeAWS:    {},
	CloudTypeYandex: {},
}

func ParseCloudType(str string) (CloudType, error) {
	ct := CloudType(str)
	if _, ok := cloudTypeMapping[ct]; !ok {
		return "", semerr.InvalidInputf("invalid cloud type: %q", str)
	}

	return ct, nil
}

var typeFQDNMapping = map[CloudType]string{
	CloudTypeAWS:    "a",
	CloudTypeYandex: "y",
}

func (t CloudType) PrefixFQDN() string {
	return typeFQDNMapping[t]
}
