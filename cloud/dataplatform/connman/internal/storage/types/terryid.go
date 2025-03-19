package types

import "a.yandex-team.ru/transfer_manager/go/pkg/terryid"

const (
	ConnectionIDPrefix = "cmc"
	OperationIDPrefix  = "cmo"
)

func GenerateConnectionID() string {
	suffix := terryid.GenerateSuffix()
	return ConnectionIDPrefix + suffix
}

func GenerateOperationID() string {
	suffix := terryid.GenerateSuffix()
	return OperationIDPrefix + suffix
}
