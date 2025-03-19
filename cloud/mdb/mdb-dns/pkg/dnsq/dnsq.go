package dnsq

import (
	"context"
)

// Client base interface for DNS Query
type Client interface {
	// return cname and ok if resolve answered, if no record found, get empty cname
	LookupCNAME(ctx context.Context, fqdn, groupid string) (cname string, ok bool)
}
