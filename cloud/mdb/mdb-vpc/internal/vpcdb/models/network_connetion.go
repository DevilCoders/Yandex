package models

import "time"

type NetworkConnection struct {
	ID           string
	NetworkID    string
	ProjectID    string
	Region       string
	Provider     Provider
	CreateTime   time.Time
	Description  string
	Status       NetworkConnectionStatus
	StatusReason string
	Params       NetworkConnectionParams
}

type NetworkConnectionStatus string

const (
	NetworkConnectionStatusCreating NetworkConnectionStatus = "CREATING"
	NetworkConnectionStatusActive   NetworkConnectionStatus = "ACTIVE"
	NetworkConnectionStatusDeleting NetworkConnectionStatus = "DELETING"
	NetworkConnectionStatusPending  NetworkConnectionStatus = "PENDING"
	NetworkConnectionStatusError    NetworkConnectionStatus = "ERROR"
)

type NetworkConnectionParams interface{}
