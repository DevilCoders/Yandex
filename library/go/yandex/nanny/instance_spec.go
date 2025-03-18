package nanny

import "a.yandex-team.ru/library/go/x/encoding/unknownjson"

type Bind struct {
	Unknown unknownjson.Store `json:"-" unknown:",store"`
}

type Resource struct {
	URL  []string `json:"url"`
	Meta Meta     `json:"fetchableMeta"`

	Unknown unknownjson.Store `json:"-" unknown:",store"`
}

type Container struct {
	Unknown unknownjson.Store `json:"-" unknown:",store"`
}

type Daemon struct {
	Unknown unknownjson.Store `json:"-" unknown:",store"`
}

type Volume struct {
	Unknown unknownjson.Store `json:"-" unknown:",store"`
}

type LayersConfig struct {
	Bind  []Bind     `json:"bind"`
	Layer []Resource `json:"layer"`

	Unknown unknownjson.Store `json:"-" unknown:",store"`
}

type InstanceSpec struct {
	ID   string `json:"id,omitempty"`
	Type string `json:"type,omitempty"`

	Containers     []Container `json:"containers"`
	InitContainers []Container `json:"initContainers"`

	AUXDaemons          []Daemon `json:"auxDaemons"`
	HostProvidedDaemons []Daemon `json:"hostProvidedDaemons"`

	Volumes      []Volume     `json:"volume"`
	LayersConfig LayersConfig `json:"layersConfig"`
	InstanceCtl  *Resource    `json:"instancectl,omitempty"`

	Unknown unknownjson.Store `json:"-" unknown:",store"`
}
