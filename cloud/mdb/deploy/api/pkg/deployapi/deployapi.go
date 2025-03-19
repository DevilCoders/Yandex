package deployapi

import (
	"context"
	"time"

	"a.yandex-team.ru/cloud/mdb/deploy/api/pkg/models"
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/ready"
	"a.yandex-team.ru/library/go/core/xerrors"
)

//go:generate ../../../../scripts/mockgen.sh Client

// Public errors
var (
	ErrNotAvailable  = xerrors.NewSentinel("mdb-deploy-api is not available")
	ErrInternalError = xerrors.NewSentinel("internal server error")
	ErrBadRequest    = xerrors.NewSentinel("bad request")
	ErrNotFound      = xerrors.NewSentinel("entity not found")
)

// Client is an interface to mdb-deploy-api service
type Client interface {
	ready.Checker

	// CreateGroup creates group
	CreateGroup(ctx context.Context, name string) (models.Group, error)
	// GetGroup retrieves group with specified name
	GetGroup(ctx context.Context, name string) (models.Group, error)
	// GetGroups retrieves list of known groups
	GetGroups(ctx context.Context, paging Paging) ([]models.Group, Paging, error)

	// CreateMaster creates master
	CreateMaster(ctx context.Context, fqdn, group string, isOpen bool, desc string) (models.Master, error)
	// UpsertMaster creates new master or updates the old one
	UpsertMaster(ctx context.Context, fqdn string, attrs UpsertMasterAttrs) (models.Master, error)
	// GetMaster retrieves master with specified fqdn
	GetMaster(ctx context.Context, fqdn string) (models.Master, error)
	// GetMasters retrieves list of known masters
	GetMasters(ctx context.Context, paging Paging) ([]models.Master, Paging, error)

	// CreateMinion creates minion
	CreateMinion(ctx context.Context, fqdn, group string, autoReassign bool) (models.Minion, error)
	// UpsertMinion creates new minion or updates the old one
	UpsertMinion(ctx context.Context, fqdn string, attrs UpsertMinionAttrs) (models.Minion, error)
	// GetMinion retrieves minion with specified fqdn
	GetMinion(ctx context.Context, fqdn string) (models.Minion, error)
	// GetMinions retrieves list of known minions
	GetMinions(ctx context.Context, paging Paging) ([]models.Minion, Paging, error)
	// GetMinionsByMaster retrieves list of minions belonging to specified master
	GetMinionsByMaster(ctx context.Context, fqdn string, paging Paging) ([]models.Minion, Paging, error)
	// GetMinionMaster retrieves minion with specified fqdn (returns only that minion's master)
	GetMinionMaster(ctx context.Context, fqdn string) (MinionMaster, error)
	// RegisterMinion registers minion's public key
	RegisterMinion(ctx context.Context, fqdn string, pubKey string) (models.Minion, error)
	// UnregisterMinion removes minion's public key and resets registration timeout allowing for new registration
	UnregisterMinion(ctx context.Context, fqdn string) (models.Minion, error)
	// DeleteMinion by fqdn
	DeleteMinion(ctx context.Context, fqdn string) error

	// CreateShipment creates shipment to run on list of hosts
	CreateShipment(
		ctx context.Context,
		fqdns []string,
		commands []models.CommandDef,
		parallel, stopOnErrorCount int64,
		timeout time.Duration,
	) (models.Shipment, error)
	// Shipment returns shipment with specified id
	GetShipment(ctx context.Context, id models.ShipmentID) (models.Shipment, error)
	// Shipments returns list of shipments
	GetShipments(ctx context.Context, attrs SelectShipmentsAttrs, paging Paging) ([]models.Shipment, Paging, error)

	// GetCommand retrieves command with specified id
	GetCommand(ctx context.Context, id models.CommandID) (models.Command, error)
	// GetCommands retrieves list of commands
	GetCommands(ctx context.Context, attrs SelectCommandsAttrs, paging Paging) ([]models.Command, Paging, error)

	// GetJob retrieves job with specified id
	GetJob(ctx context.Context, id models.JobID) (models.Job, error)
	// GetJobs retrieves list of jobs
	GetJobs(ctx context.Context, attrs SelectJobsAttrs, paging Paging) ([]models.Job, Paging, error)

	// CreateJobResult creates job result for specified job saving provided payload
	CreateJobResult(ctx context.Context, id, fqdn string, result string) (models.JobResult, error)
	// GetJobResult retrieves job result with specified id
	GetJobResult(ctx context.Context, id models.JobResultID) (models.JobResult, error)
	// GetJobResults returns list of job results for specific job id and fqdn
	GetJobResults(ctx context.Context, attrs SelectJobResultsAttrs, paging Paging) ([]models.JobResult, Paging, error)
}

// MinionMaster holds minion's public key and its master fqdn
type MinionMaster struct {
	MasterFQDN string
	PublicKey  string
}

// UpsertMasterAttrs ...
type UpsertMasterAttrs struct {
	Group       optional.String
	IsOpen      optional.Bool
	Description optional.String
}

// UpsertMinionAttrs ...
type UpsertMinionAttrs struct {
	Group        optional.String
	AutoReassign optional.Bool
	Master       optional.String
}

// SelectShipmentsAttrs ...
type SelectShipmentsAttrs struct {
	FQDN   optional.String
	Status optional.String
}

// SelectCommandsAttrs ...
type SelectCommandsAttrs struct {
	ShipmentID optional.String
	FQDN       optional.String
	Status     optional.String
}

// SelectJobsAttrs ...
type SelectJobsAttrs struct {
	ShipmentID optional.String
	FQDN       optional.String
	ExtJobID   optional.String
	Status     optional.String
}

// SelectJobResultsAttrs ...
type SelectJobResultsAttrs struct {
	ExtJobID optional.String
	FQDN     optional.String
	Status   optional.String
}

// Paging defines page size and token values. Default values are safe to use.
type Paging struct {
	// Size is pageSize
	Size int64
	// Token is pageToken
	Token string
	// SortOrder of this paging
	SortOrder models.SortOrder
}

// HasSortOrder returns true if sort order is set
func (p Paging) HasSortOrder() bool {
	return p.SortOrder != models.SortOrderUnknown && p.SortOrder != ""
}
