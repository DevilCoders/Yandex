package instance

import (
	"encoding/json"
	"io"
	"sort"
	"strconv"
	"text/template"

	"a.yandex-team.ru/library/go/core/xerrors"
)

// Mostly copied from a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/compute/v1/instance.pb.go
//
// Changed all number types to strings
type Instance struct {
	ID   string `protobuf:"bytes,1,opt,name=id,proto3" json:"id,omitempty"`
	Name string `protobuf:"bytes,4,opt,name=name,proto3" json:"name,omitempty"`
	//Description       string               `protobuf:"bytes,5,opt,name=description,proto3" json:"description,omitempty"`
	//Labels            map[string]string    `protobuf:"bytes,6,rep,name=labels,proto3" json:"labels,omitempty" protobuf_key:"bytes,1,opt,name=key,proto3" protobuf_val:"bytes,2,opt,name=value,proto3"`
	ZoneID     string `protobuf:"bytes,7,opt,name=zone_id,json=zoneId,proto3" json:"zone_id,omitempty"`
	PlatformID string `protobuf:"bytes,8,opt,name=platform_id,json=platformId,proto3" json:"platform_id,omitempty"`
	Resources  struct {
		Memory       string `protobuf:"varint,1,opt,name=memory,proto3" json:"memory,omitempty"`
		Cores        string `protobuf:"varint,2,opt,name=cores,proto3" json:"cores,omitempty"`
		CoreFraction string `protobuf:"varint,3,opt,name=core_fraction,json=coreFraction,proto3" json:"core_fraction,omitempty"`
		Gpus         string `protobuf:"varint,4,opt,name=gpus,proto3" json:"gpus,omitempty"`
		Sockets      string `protobuf:"varint,5,opt,name=sockets,proto3" json:"sockets,omitempty"`
		NvmeDisks    string `protobuf:"varint,100,opt,name=nvme_disks,json=nvmeDisks,proto3" json:"nvme_disks,omitempty"`
	} `protobuf:"bytes,9,opt,name=resources,proto3" json:"resources,omitempty"`
	Metadata map[string]string `protobuf:"bytes,11,rep,name=metadata,proto3" json:"metadata,omitempty" protobuf_key:"bytes,1,opt,name=key,proto3" protobuf_val:"bytes,2,opt,name=value,proto3"`
	BootDisk struct {
		DiskID string `protobuf:"bytes,4,opt,name=disk_id,json=diskId,proto3" json:"disk_id,omitempty"`
	} `protobuf:"bytes,12,opt,name=boot_disk,json=bootDisk,proto3" json:"boot_disk,omitempty"`
	//SecondaryDisks    []*AttachedDisk     `protobuf:"bytes,13,rep,name=secondary_disks,json=secondaryDisks,proto3" json:"secondary_disks,omitempty"`
	NetworkInterfaces []struct {
		Index      string `protobuf:"bytes,1,opt,name=index,proto3" json:"index,omitempty"`
		MacAddress string `protobuf:"bytes,2,opt,name=mac_address,json=macAddress,proto3" json:"mac_address,omitempty"`
		SubnetID   string `protobuf:"bytes,3,opt,name=subnet_id,json=subnetId,proto3" json:"subnet_id,omitempty"`
		//PrimaryV4Address     *PrimaryAddress `protobuf:"bytes,4,opt,name=primary_v4_address,json=primaryV4Address,proto3" json:"primary_v4_address,omitempty"`
		//PrimaryV6Address     *PrimaryAddress `protobuf:"bytes,5,opt,name=primary_v6_address,json=primaryV6Address,proto3" json:"primary_v6_address,omitempty"`
		SecurityGroupIDs []string `protobuf:"bytes,6,rep,name=security_group_ids,json=securityGroupIds,proto3" json:"security_group_ids,omitempty"`
	} `protobuf:"bytes,14,rep,name=network_interfaces,json=networkInterfaces,proto3" json:"network_interfaces,omitempty"`
	Hostname string `protobuf:"bytes,15,opt,name=hostname,proto3" json:"hostname,omitempty"`
	Fqdn     string `protobuf:"bytes,16,opt,name=fqdn,proto3" json:"fqdn,omitempty"`
}

var configTemplate = template.Must(template.New("instanceConfig").Parse(
	`resource ycp_compute_instance {{ .Name }} {
  name        = "{{ .Name }}"
  hostname    = "{{ .Hostname }}"
  zone_id     = "{{ .ZoneID }}"
  platform_id = "{{ .PlatformID }}"
  fqdn        = "{{ .Fqdn }}"

  resources {
    core_fraction = {{ .Resources.CoreFraction }}
    cores         = {{ .Resources.Cores }}
    memory        = {{ .Resources.Memory }}
  }

  boot_disk {
    disk_spec {
      name        = data.yandex_compute_disk.{{ .Name }}_boot.name
      size        = data.yandex_compute_disk.{{ .Name }}_boot.size
      image_id    = data.yandex_compute_disk.{{ .Name }}_boot.image_id
      snapshot_id = data.yandex_compute_disk.{{ .Name }}_boot.snapshot_id
      type_id     = data.yandex_compute_disk.{{ .Name }}_boot.type
    }
  }

  network_interface {
    subnet_id = "{{ (index .NetworkInterfaces 0).SubnetID }}"
    primary_v6_address {}
  }
}
data "yandex_compute_disk" "{{ .Name }}_boot" {
  disk_id = "{{ .BootDisk.DiskID }}"
}
`))

var importTemplate = template.Must(template.New("instanceImport").Parse(
	`terraform import {{- if .VarFile }} -var-file {{ .VarFile }} {{- end }} ycp_compute_instance.{{ .Name }} {{ .ID }}
`))

func Instances(in []byte, out io.WriteCloser) error {
	defer out.Close()
	parsed, err := parseInstances(in)
	if err != nil {
		return err
	}
	for _, instance := range parsed {
		if err := configTemplate.Execute(out, instance); err != nil {
			return err
		}
	}
	return nil
}

func InstanceImports(in []byte, out io.WriteCloser, varFile string) error {
	defer out.Close()
	parsed, err := parseInstances(in)
	if err != nil {
		return err
	}
	for _, instance := range parsed {
		if err := importTemplate.Execute(out, struct {
			VarFile string
			Name    string
			ID      string
		}{
			VarFile: varFile,
			Name:    instance.Name,
			ID:      instance.ID,
		}); err != nil {
			return err
		}
	}

	return nil
}

func parseInstances(in []byte) ([]*Instance, error) {
	var parsed []*Instance

	if err := json.Unmarshal(in, &parsed); err != nil {
		return nil, xerrors.Errorf("parse instance: %w", err)
	}
	sort.Sort(sortableInstances(parsed))
	for i := range parsed {
		memBytes, err := strconv.Atoi(parsed[i].Resources.Memory)
		if err != nil {
			return nil, err
		}
		parsed[i].Resources.Memory = strconv.Itoa(memBytes / 1024 / 1024 / 1024)
	}
	return parsed, nil
}

type sortableInstances []*Instance

func (s sortableInstances) Len() int {
	return len(s)
}

func (s sortableInstances) Less(i, j int) bool {
	return s[i].Name < s[j].Name
}

func (s sortableInstances) Swap(i, j int) {
	s[i], s[j] = s[j], s[i]
}
