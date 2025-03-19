package role

import (
	"strings"

	"a.yandex-team.ru/library/go/core/xerrors"
)

// Role is a subcluster role enum
type Role int

const (
	// Main - main subcluster
	Main Role = iota
	// Data - datanodes subcluster
	Data
	// Compute - computenodes subcluster
	Compute
	// Instance group based subcluster
	ComputeAutoScaling
	Unknown
)

func (role Role) String() string {
	names := [...]string{
		"Main",
		"Data",
		"Compute",
		"ComputeAutoScaling",
		"Unknown"}

	return names[role]
}

func DeduceRoleByNodeFQDN(fqdn string) (Role, error) {
	domainParts := strings.Split(fqdn, ".")
	if len(domainParts) > 0 {
		subdomain := domainParts[0]
		subdomainParts := strings.Split(subdomain, "-")
		if len(subdomainParts) > 2 {
			switch subdomainParts[2] {
			case "m":
				return Main, nil
			case "d":
				return Data, nil
			case "c":
				return Compute, nil
			case "g":
				return ComputeAutoScaling, nil
			default:
				return Unknown, xerrors.Errorf("can not deduce node role type from fqdn: %s", fqdn)
			}
		}
	}
	return Unknown, xerrors.Errorf("can not deduce node role type from fqdn: %s", fqdn)
}
