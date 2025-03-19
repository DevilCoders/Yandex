package push

import "context"

//go:generate ../../../scripts/mockgen.sh Pusher

type Event struct {
	Host        string   `json:"host"`
	Service     string   `json:"service"`
	Instance    string   `json:"instance,omitempty"`
	Status      Status   `json:"status"`
	Description string   `json:"description,omitempty"`
	Tags        []string `json:"tags,omitempty"`
}

type Status string

const (
	OK   Status = "OK"
	CRIT Status = "CRIT"
	WARN Status = "WARN"
)

type Request struct {
	Source string  `json:"source"`
	Events []Event `json:"events"`
}

type Pusher interface {
	Push(ctx context.Context, request Request) error
}
