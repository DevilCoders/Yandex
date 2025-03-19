package models

import "time"

type Network struct {
	ID                string
	ProjectID         string
	Provider          Provider
	Region            string
	CreateTime        time.Time
	Name              string
	Description       string
	IPv4              string
	IPv6              string
	Status            NetworkStatus
	StatusReason      string
	ExternalResources ExternalResources
}

func (n *Network) IsImported() bool {
	return n.ExternalResources.IsImported()
}

type NetworkStatus string

const (
	NetworkStatusCreating NetworkStatus = "CREATING"
	NetworkStatusActive   NetworkStatus = "ACTIVE"
	NetworkStatusDeleting NetworkStatus = "DELETING"
)

type ExternalResources interface {
	IsImported() bool
}
