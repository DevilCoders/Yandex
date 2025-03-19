package conductor

import (
	"context"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
)

//go:generate ../../scripts/mockgen.sh Client

// Client to conductor
type Client interface {
	GroupToHosts(ctx context.Context, group string, attrs GroupToHostsAttrs) ([]string, error)
	HostToGroups(ctx context.Context, host string) ([]string, error)
	HostsInfo(ctx context.Context, hosts []string) ([]HostInfo, error)
	GroupInfoByName(ctx context.Context, group string) (GroupInfo, error)
	ParentGroup(ctx context.Context, groupInfo GroupInfo) (GroupInfo, error)
	DCByName(ctx context.Context, name string) (DataCenter, error)
	HostCreate(ctx context.Context, request HostCreateRequest) error
	ExecuterData(ctx context.Context, projectName string) (ExecuterData, error)
}

type HostCreateRequest struct {
	FQDN         string
	ShortName    string
	GroupID      int
	DataCenterID int
}

type DataCenter struct {
	ID int
}

type GroupToHostsAttrs struct {
	DC optional.String
}

type HostInfo struct {
	ID          int      `json:"id"`
	Group       string   `json:"group"`
	FQDN        string   `json:"fqdn"`
	DC          string   `json:"root_datacenter"`
	ShortName   string   `json:"short_name"`
	Discription string   `json:"discription"`
	Admins      []string `json:"admins"`
}

type GroupInfo struct {
	Name      string
	URLParent string
}

type HostExecuterData struct {
	Group string `json:"group"`
}

type GroupExecuterData struct {
	Parents []string `json:"parents"`
}

type ExecuterData struct {
	Hosts  map[string]HostExecuterData  `json:"Hosts"`
	Groups map[string]GroupExecuterData `json:"Groups"`
}
