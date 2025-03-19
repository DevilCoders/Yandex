package dns

import "net"

//go:generate ../../../scripts/mockgen.sh Resolver

type Resolver interface {
	LookupHost(host string) (addrs []string, err error)
}

type DefaultResolver struct {
}

func (r *DefaultResolver) LookupHost(host string) (addrs []string, err error) {
	return net.LookupHost(host)
}

func NewDefaultResolver() Resolver {
	return &DefaultResolver{}
}
