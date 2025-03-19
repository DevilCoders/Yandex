package protoutil

import (
	"google.golang.org/protobuf/encoding/protojson"
	"google.golang.org/protobuf/proto"

	"a.yandex-team.ru/cloud/mdb/internal/secret"
)

func MaskJSONMessage(m proto.Message) (string, error) {
	body := protojson.Format(m)
	res, err := secret.MaskKeysInJSON([]byte(body), secret.DefaultSecretKeys)
	if err != nil {
		return "", err
	}

	return string(res), nil
}
