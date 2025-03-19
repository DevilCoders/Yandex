package balancer

import (
	"encoding/json"
	"io"
	"sort"
	"text/template"

	"a.yandex-team.ru/library/go/core/xerrors"
)

var configTemplate = template.Must(template.New("targetGroupConfig").Parse(
	`resource "ycp_load_balancer_target_group" "{{ .Name }}" {
  name      = "{{ .Name }}"
  region_id = var.region_id
  target {
    address   = ycp_compute_instance.{{ .Name }}-{{ .Env }}01f.network_interface.0.primary_v6_address[0].address
    subnet_id = ycp_compute_instance.{{ .Name }}-{{ .Env }}01f.network_interface.0.subnet_id
  }
  target {
    address   = ycp_compute_instance.{{ .Name }}-{{ .Env }}01k.network_interface.0.primary_v6_address[0].address
    subnet_id = ycp_compute_instance.{{ .Name }}-{{ .Env }}01k.network_interface.0.subnet_id
  }
  target {
    address   = ycp_compute_instance.{{ .Name }}-{{ .Env }}01h.network_interface.0.primary_v6_address[0].address
    subnet_id = ycp_compute_instance.{{ .Name }}-{{ .Env }}01h.network_interface.0.subnet_id
  }
}
`))

var importTemplate = template.Must(template.New("targetGroupImport").Parse(
	`terraform import {{- if .VarFile }} -var-file {{ .VarFile }} {{- end }} ycp_load_balancer_target_group.{{ .Name }} {{ .ID }}
`))

type TargetGroup struct {
	ID       string `protobuf:"bytes,1,opt,name=id,proto3" json:"id,omitempty"`
	FolderID string `protobuf:"bytes,2,opt,name=folder_id,json=folderId,proto3" json:"folder_id,omitempty"`
	//CreatedAt              *timestamp.Timestamp `protobuf:"bytes,3,opt,name=created_at,json=createdAt,proto3" json:"created_at,omitempty"`
	Name        string            `protobuf:"bytes,4,opt,name=name,proto3" json:"name,omitempty"`
	Description string            `protobuf:"bytes,5,opt,name=description,proto3" json:"description,omitempty"`
	Labels      map[string]string `protobuf:"bytes,6,rep,name=labels,proto3" json:"labels,omitempty" protobuf_key:"bytes,1,opt,name=key,proto3" protobuf_val:"bytes,2,opt,name=value,proto3"`
	RegionID    string            `protobuf:"bytes,7,opt,name=region_id,json=regionId,proto3" json:"region_id,omitempty"`
	Targets     []*struct {
		SubnetID string `protobuf:"bytes,1,opt,name=subnet_id,json=subnetId,proto3" json:"subnet_id,omitempty"`
		Address  string `protobuf:"bytes,2,opt,name=address,proto3" json:"address,omitempty"`
	} `protobuf:"bytes,9,rep,name=targets,proto3" json:"targets,omitempty"`
	NetworkLoadBalancerIDs []string `protobuf:"bytes,10,rep,name=network_load_balancer_ids,json=networkLoadBalancerIds,proto3" json:"network_load_balancer_ids,omitempty"`
}

func TargetGroups(in []byte, out io.WriteCloser, env string) error {
	defer out.Close()
	parsed, err := parseTargetGroups(in)
	if err != nil {
		return err
	}
	for _, tg := range parsed {
		if err := configTemplate.Execute(out, struct {
			*TargetGroup
			Env string
		}{TargetGroup: tg, Env: env}); err != nil {
			return err
		}
	}
	return nil
}

func TargetGroupsImports(in []byte, out io.WriteCloser, varFile string) error {
	defer out.Close()
	parsed, err := parseTargetGroups(in)
	if err != nil {
		return err
	}
	for _, tg := range parsed {
		if err := importTemplate.Execute(out, struct {
			VarFile string
			Name    string
			ID      string
		}{
			VarFile: varFile,
			Name:    tg.Name,
			ID:      tg.ID,
		}); err != nil {
			return err
		}
	}

	return nil
}

func parseTargetGroups(in []byte) ([]*TargetGroup, error) {
	var parsed []*TargetGroup

	if err := json.Unmarshal(in, &parsed); err != nil {
		return nil, xerrors.Errorf("parse instance: %w", err)
	}
	sort.Sort(sortableTargetGroups(parsed))
	return parsed, nil
}

type sortableTargetGroups []*TargetGroup

func (s sortableTargetGroups) Len() int {
	return len(s)
}

func (s sortableTargetGroups) Less(i, j int) bool {
	return s[i].Name < s[j].Name
}

func (s sortableTargetGroups) Swap(i, j int) {
	s[i], s[j] = s[j], s[i]
}
