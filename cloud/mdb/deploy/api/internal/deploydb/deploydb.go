package deploydb

import (
	"context"
	"io"
	"sync"
	"time"

	"github.com/opentracing/opentracing-go"

	"a.yandex-team.ru/cloud/mdb/deploy/api/pkg/models"
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/ready"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

// CommandDispatcher retrieves commands for dispatching. Closing interface is mandatory.
type CommandDispatcher interface {
	Close(ctx context.Context) error

	// MasterFQDN returns fqdn of master these commands are destined for
	MasterFQDN() string
	// Commands returns commands which can be dispatched
	Commands() []models.Command
	// Dispatched marks command as dispatched
	Dispatched(ctx context.Context, cmdID models.CommandID, jobID string) (models.Job, error)
	// DispatchFailed for specific command
	DispatchFailed(ctx context.Context, id models.CommandID) (models.Command, error)
}

type RunningJob struct {
	ExtJobID string
	Minion   string
}

// RunningJobsChecker retrieves running jobs for checking.
type RunningJobsChecker interface {
	// Jobs returns jobs which should be checked
	Jobs() []RunningJob
	// NotRunning marks job as not running
	NotRunning(ctx context.Context, job RunningJob, failOnCount int) (models.Job, error)
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
	FQDN      optional.String
	Status    optional.String
	SortOrder models.SortOrder
}

// SelectCommandsAttrs ...
type SelectCommandsAttrs struct {
	ShipmentID optional.Int64
	FQDN       optional.String
	Status     optional.String
	SortOrder  models.SortOrder
}

// SelectJobsAttrs ...
type SelectJobsAttrs struct {
	ShipmentID optional.Int64
	FQDN       optional.String
	ExtJobID   optional.String
	Status     optional.String
	SortOrder  models.SortOrder
}

// SelectJobResultsAttrs ...
type SelectJobResultsAttrs struct {
	ExtJobID  optional.String
	FQDN      optional.String
	Status    optional.String
	SortOrder models.SortOrder
}

// Backend interface to deploydb
type Backend interface {
	io.Closer
	ready.Checker

	sqlutil.TxBinder

	// CreateGroup creates group
	CreateGroup(ctx context.Context, name string) (models.Group, error)
	// Group returns group with specified name
	Group(ctx context.Context, name string) (models.Group, error)
	// Groups returns known groups
	Groups(ctx context.Context, sortOrder models.SortOrder, limit int64, lastGroupID optional.Int64) ([]models.Group, error)

	// CreateMaster creates master
	CreateMaster(ctx context.Context, fqdn, group string, isOpen bool, desc string) (models.Master, error)
	// UpsertMaster creates new master or updates the old one
	UpsertMaster(ctx context.Context, fqdn string, attrs UpsertMasterAttrs) (models.Master, error)
	// Master returns master with specified fqdn
	Master(ctx context.Context, fqdn string) (models.Master, error)
	// Masters returns known masters
	Masters(ctx context.Context, limit int64, lastMasterID optional.Int64) ([]models.Master, error)
	// UpdateMasterCheck updates checker's view of specified master and returns current info about that master
	UpdateMasterCheck(ctx context.Context, masterFQDN, checkerFQDN string, alive bool) (models.Master, error)

	// CreateMinion creates minion
	CreateMinion(ctx context.Context, fqdn, group string, autoReassign bool) (models.Minion, error)
	// UpsertMinion creates new minion or updates the old one
	UpsertMinion(ctx context.Context, fqdn string, attrs UpsertMinionAttrs, allowRecreate bool) (models.Minion, error)
	// Minion returns minion with specified fqdn
	Minion(ctx context.Context, fqdn string) (models.Minion, error)
	// Minions returns known minions
	Minions(ctx context.Context, limit int64, lastMinionID optional.Int64) ([]models.Minion, error)
	// MinionsByMaster returns minions assigned to specified master
	MinionsByMaster(
		ctx context.Context,
		fqdn string,
		limit int64,
		lastMinionID optional.Int64,
	) ([]models.Minion, error)
	// RegisterMinion registers minion's public key
	RegisterMinion(ctx context.Context, fqdn string, pubKey string) (models.Minion, error)
	// UnregisterMinion removes minion's public key and resets registration timeout allowing for new registration
	UnregisterMinion(ctx context.Context, fqdn string) (models.Minion, error)
	// DeleteMinion by fqdn
	DeleteMinion(ctx context.Context, fqdn string) error
	// FailoverMinions (specified number of minions)
	FailoverMinions(ctx context.Context, limit int64) ([]models.Minion, error)

	// CreateShipment creates shipment to run on list of hosts
	CreateShipment(
		ctx context.Context,
		fqdns []string,
		skipFQDNs []string,
		commands []models.CommandDef,
		parallel, stopOnErrorCount int64,
		timeout time.Duration,
		carrier opentracing.TextMapCarrier,
	) (models.Shipment, error)
	// Shipment returns shipment with specified id
	Shipment(ctx context.Context, id models.ShipmentID) (models.Shipment, error)
	// Shipments returns list of shipments
	Shipments(ctx context.Context, attrs SelectShipmentsAttrs, limit int64, lastShipmentID optional.Int64) ([]models.Shipment, error)
	// TimeoutShipments that passed their timeout
	TimeoutShipments(ctx context.Context, limit int64) ([]models.Shipment, error)

	// Command returns command with specified id
	Command(ctx context.Context, id models.CommandID) (models.Command, error)
	// Commands returns list of commands
	Commands(ctx context.Context, attrs SelectCommandsAttrs, limit int64, lastCommandID optional.Int64) ([]models.Command, error)
	// CommandsForDispatch returns list of commands ready for dispatch. DO NOT forget to close returned interface
	CommandsForDispatch(ctx context.Context, fqdn string, limit int64, throttlingCount int32) (CommandDispatcher, error)

	// Job retrieves job with specified id
	Job(ctx context.Context, id models.JobID) (models.Job, error)
	// Jobs returns list of jobs
	Jobs(ctx context.Context, attrs SelectJobsAttrs, limit int64, lastJobID optional.Int64) ([]models.Job, error)
	// RunningJobsForCheck that should be checked
	RunningJobsForCheck(ctx context.Context, master string, running time.Duration, limit int64) (RunningJobsChecker, error)
	// TimeoutJobs that passed their timeout
	TimeoutJobs(ctx context.Context, limit int64) ([]models.Job, error)

	// CreateJobResult creates job result with for specified job
	CreateJobResult(ctx context.Context, id, fqdn string, status models.JobResultStatus, result []byte) (models.JobResult, error)
	// JobResult returns job result with specified id
	JobResult(ctx context.Context, id models.JobResultID) (models.JobResult, error)
	// JobResults returns list of job results for specified job id and fqdn
	JobResults(ctx context.Context, attrs SelectJobResultsAttrs, limit int64, lastJobResultID optional.Int64) ([]models.JobResult, error)
	JobResultCoords(ctx context.Context, jobExtID, fqdn string) (models.JobResultCoords, error)

	// TODO Use separete interface for Cleanup* methods
	// CleanupUnboundJobResults removes job results which was created without shipments
	CleanupUnboundJobResults(ctx context.Context, age time.Duration, limit uint64) (uint64, error)
	// CleanupShipments removes shipments and all corresponded data
	CleanupShipments(ctx context.Context, age time.Duration, limit uint64) (uint64, error)
	// CleanupMinionsWithoutJobs removes all deleted minions without jobs and shipments and their change logs
	CleanupMinionsWithoutJobs(ctx context.Context, age time.Duration, limit uint64) (uint64, error)
}

type factoryFunc func(logger log.Logger) (Backend, error)

var (
	backendsMu sync.RWMutex
	backends   = make(map[string]factoryFunc)
)

// RegisterBackend registers new db backend named 'name'
func RegisterBackend(name string, factory factoryFunc) {
	backendsMu.Lock()
	defer backendsMu.Unlock()

	if factory == nil {
		panic("deploydb: register backend is nil")
	}

	if _, dup := backends[name]; dup {
		panic("deploydb: register backend called twice for driver " + name)
	}

	backends[name] = factory
}

// Open constructs new db backend named 'name'
func Open(name string, logger log.Logger) (Backend, error) {
	backendsMu.RLock()
	backend, ok := backends[name]
	backendsMu.RUnlock()
	if !ok {
		return nil, xerrors.Errorf("unknown deploydb backend: %s", name)
	}

	return backend(logger)
}
