package dnsapi

import (
	"context"

	"github.com/opentracing/opentracing-go"

	"a.yandex-team.ru/cloud/mdb/internal/grpcutil"
	"a.yandex-team.ru/cloud/mdb/internal/secret"
	"a.yandex-team.ru/library/go/core/xerrors"
)

// Errors
var (
	ErrRequestFailed = xerrors.NewSentinel("failed request to DNS api")
	ErrInternal      = xerrors.NewSentinel("internal dnaapi client error")
)

// Config base configuration for dns api
type Config struct {
	Baseurl     string                `json:"baseurl" yaml:"baseurl"`
	GRPCurl     string                `json:"grpcurl" yaml:"grpcurl"`
	Account     string                `json:"account" yaml:"account"`
	Token       secret.String         `json:"token" yaml:"token"`
	FQDNSuffix  string                `json:"fqdnsuffix" yaml:"fqdnsuffix"`
	ResolveNS   string                `json:"resolvens" yaml:"resolvens"`
	MaxRec      uint                  `json:"maxrec" yaml:"maxrec"`
	UpdThrs     uint                  `json:"updthreads" yaml:"updthreads"`
	LogHTTPBody bool                  `json:"loghttpbody" yaml:"loghttpbody"`
	GRPCCfg     grpcutil.ClientConfig `json:"grpccfg" yaml:"grpccfg"`
}

// Update update CNAME information, it can be used for add\delete if empty
type Update struct {
	CNAMEOld    string
	CNAMENew    string
	RequestSpan opentracing.Span
}

// Records is map basefqdn->Update
type Records map[string]Update

// RequestUpdate handles all logic of service
type RequestUpdate struct {
	Records
	GroupID string // only significant for compute
}

// Client base interface for DNS API
type Client interface {
	// update few records at once, on error returns partial lists for update
	UpdateRecords(ctx context.Context, up *RequestUpdate) ([]*RequestUpdate, error)
}

// SplitOnParts split batch request update on max 5 smaller parts
func SplitOnParts(ru *RequestUpdate) []*RequestUpdate {
	if len(ru.Records) < 2 {
		return nil
	}
	numParts := int(5)
	maxInPart := (len(ru.Records) + numParts - 1) / numParts
	partRecs := len(ru.Records)
	if partRecs > numParts {
		partRecs = numParts
	}
	resList := make([]*RequestUpdate, partRecs)
	for i := range resList {
		resList[i] = &RequestUpdate{
			GroupID: ru.GroupID,
			Records: make(Records, maxInPart),
		}
	}
	i := 0
	for k, v := range ru.Records {
		resList[i%numParts].Records[k] = v
		i++
	}

	return resList
}
