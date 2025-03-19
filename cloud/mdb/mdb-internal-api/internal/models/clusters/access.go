package clusters

import (
	"fmt"
	"net"
	"strings"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/reflectutil"
	"a.yandex-team.ru/library/go/core/xerrors"
)

type AccessDataService int

const (
	AccessInvalid AccessDataService = iota
	AccessVisualization
	AccessTransfer
)

var (
	mapDataServiceToString = map[AccessDataService]string{
		AccessInvalid:       "INVALID",
		AccessVisualization: "DATA-LENS",
		AccessTransfer:      "DATA-TRANSFER",
	}

	nameToDataServiceMapping = reflectutil.ReverseMap(mapDataServiceToString).(map[string]AccessDataService)
)

func (s AccessDataService) String() string {
	str, ok := mapDataServiceToString[s]
	if !ok {
		return fmt.Sprintf("INVALID_DATA_SERVICE_%d", s)
	}
	return str
}

func ParseAccessDataService(str string) (AccessDataService, error) {
	s, ok := nameToDataServiceMapping[strings.ToUpper(str)]
	if !ok {
		return AccessInvalid, xerrors.Errorf("unknown data service %q", str)
	}
	return s, nil
}

type CidrBlock struct {
	Value       string `json:"value"`
	Description string `json:"description"`
}

type Access struct {
	DataLens       optional.Bool
	WebSQL         optional.Bool
	Metrica        optional.Bool
	Serverless     optional.Bool
	DataTransfer   optional.Bool
	YandexQuery    optional.Bool
	Ipv4CidrBlocks []CidrBlock
	Ipv6CidrBlocks []CidrBlock
}

func (acc *Access) ValidateAndSane() error {
	for _, ipStr := range acc.Ipv4CidrBlocks {
		if err := ValidateCIDRString(ipStr.Value, IPv4CIDR); err != nil {
			return err
		}
	}
	for _, ipStr := range acc.Ipv6CidrBlocks {
		if err := ValidateCIDRString(ipStr.Value, IPv6CIDR); err != nil {
			return err
		}
	}
	return nil
}

type IPType int

const (
	IPv4CIDR IPType = iota
	IPv6CIDR
)

func ValidateCIDRString(str string, ipType IPType) error {
	ip, _, err := net.ParseCIDR(str)
	if err != nil {
		return xerrors.Errorf("provided string could not be parsed as valid IP address in CIDR notation: %s", str)
	}
	if ipType == IPv4CIDR && ip.To4() != nil && strings.Count(str, ":") < 2 {
		return nil
	}
	if ipType == IPv6CIDR && strings.Count(str, ":") >= 2 {
		return nil
	}
	addressTypeStr := "IPv6 CIDR"
	if ipType == IPv4CIDR {
		addressTypeStr = "IPv4 CIDR"
	}
	return xerrors.Errorf("incorrect address type; got %s expected %s", str, addressTypeStr)
}
