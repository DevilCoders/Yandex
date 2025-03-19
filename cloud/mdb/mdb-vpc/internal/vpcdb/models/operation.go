package models

import (
	"time"
)

type Provider string
type OperationAction string
type OperationStatus string

const (
	ProviderAWS Provider = "aws"

	OperationActionCreateVPC OperationAction = "create_vpc"
	OperationActionDeleteVPC OperationAction = "delete_vpc"
	OperationActionImportVPC OperationAction = "import_vpc"

	OperationActionCreateNetworkConnection OperationAction = "create_network_connection"
	OperationActionDeleteNetworkConnection OperationAction = "delete_network_connection"

	OperationStatusPending OperationStatus = "PENDING"
	OperationStatusRunning OperationStatus = "RUNNING"
	OperationStatusDone    OperationStatus = "DONE"
)

type Operation struct {
	ID          string
	Provider    Provider
	Region      string
	ProjectID   string
	Description string
	Params      OperationParams
	State       OperationState
	Action      OperationAction
	StartTime   time.Time
	FinishTime  time.Time
	Status      OperationStatus
}

type OperationParams interface{}

type CreateNetworkOperationParams struct {
	NetworkID string `json:"network_id"`
}

type DeleteNetworkOperationParams struct {
	NetworkID string `json:"network_id"`
}

type CreateNetworkConnectionOperationParams struct {
	NetworkConnectionID string `json:"network_connection_id"`
}

type DeleteNetworkConnectionOperationParams struct {
	NetworkConnectionID string `json:"network_connection_id"`
}

func DefaultCreateNetworkOperationParams() OperationParams {
	return &CreateNetworkOperationParams{}
}

func DefaultDeleteNetworkOperationParams() OperationParams {
	return &DeleteNetworkOperationParams{}
}

func DefaultCreateNetworkConnectionOperationParams() OperationParams {
	return &CreateNetworkConnectionOperationParams{}
}

func DefaultDeleteNetworkConnectionOperationParams() OperationParams {
	return &DeleteNetworkConnectionOperationParams{}
}

type OperationState interface{}
