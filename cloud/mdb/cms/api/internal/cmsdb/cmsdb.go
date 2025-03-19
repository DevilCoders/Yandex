package cmsdb

import (
	"context"
	"io"
	"time"

	"a.yandex-team.ru/cloud/mdb/cms/api/pkg/models"
	"a.yandex-team.ru/cloud/mdb/internal/ready"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/library/go/core/xerrors"
)

//go:generate ../../../../scripts/mockgen.sh Client

type RequestToCreate struct {
	Name         string
	ExtID        string
	Comment      string
	Author       string
	RequestType  string
	Fqnds        []string
	Extra        interface{}
	FailureType  string
	ScenarioInfo models.ScenarioInfo
}

var ErrLockNotTaken = xerrors.NewSentinel("failed to get lock")

type Client interface {
	ready.Checker
	io.Closer
	sqlutil.TxBinder
	GetLock(ctx context.Context, key LockKey) error
	// requests
	GetRequests(ctx context.Context) ([]models.ManagementRequest, error)
	MarkRequestsDeletedByTaskID(ctx context.Context, taskIDs []string) ([]string, error)
	CreateRequests(ctx context.Context, createUs []RequestToCreate) (models.RequestStatus, error)
	GetRequestsByTaskID(ctx context.Context, taskIDs []string) (map[string]models.ManagementRequest, error)
	GetRequestsByID(ctx context.Context, ids []int64) (map[int64]models.ManagementRequest, error)
	GetRequestsWithDeletedByID(ctx context.Context, ids []int64) (map[int64]models.ManagementRequest, error)
	UpdateRequestFields(ctx context.Context, r models.ManagementRequest) error
	// decisions
	CreateDecision(ctx context.Context, requestID int64, status models.DecisionStatus, explanation string) (int64, error)
	GetDecisionsByID(ctx context.Context, decisionIDs []int64) (map[int64]models.AutomaticDecision, error)
	GetDecisionsByRequestID(ctx context.Context, IDs []int64) ([]models.AutomaticDecision, error)
	GetNotFinishedDecisionsByFQDN(ctx context.Context, fqdns []string) ([]models.AutomaticDecision, error)
	GetDecisionsToProcess(ctx context.Context, skipIDs []int64) (models.AutomaticDecision, error)
	GetDecisionsToLetGo(ctx context.Context, skipIDs []int64) (models.AutomaticDecision, error)
	GetDecisionsToReturnFromWalle(ctx context.Context, skipIDs []int64) (models.AutomaticDecision, error)
	GetDecisionsToFinishAfterWalle(ctx context.Context, skipIDs []int64) (models.AutomaticDecision, error)
	GetDecisionsToCleanup(ctx context.Context, skipIDs []int64) (models.AutomaticDecision, error)
	MoveDecisionsToStatus(ctx context.Context,
		decisionIDs []int64,
		status models.DecisionStatus) error
	UpdateDecisionFields(
		ctx context.Context,
		d models.AutomaticDecision) error
	MarkRequestsAnalysedByAutoDuty(ctx context.Context, ds []models.AutomaticDecision) error
	MarkRequestsCameBack(ctx context.Context, ds []models.AutomaticDecision) error
	MarkRequestsResolvedByAutoDuty(ctx context.Context, ds []models.AutomaticDecision) error
	MarkRequestsFinishedByAutoDuty(ctx context.Context, ds []models.AutomaticDecision) error
	SetAutoDutyResolution(ctx context.Context, decisionIDs []int64, res models.AutoResolution) error
	// statistics
	GetRequestsToConsider(ctx context.Context, threshold int) ([]models.ManagementRequest, error)
	GetRequestsStatInWindow(ctx context.Context, window time.Duration) ([]models.ManagementRequest, error)
	GetUnfinishedRequests(ctx context.Context, window time.Duration) ([]models.ManagementRequest, error)
	GetResetupRequests(ctx context.Context, window time.Duration) ([]models.ManagementRequest, error)
	// instances
	CreateInstanceOperation(
		ctx context.Context,
		idempotencyKey string,
		operationType models.InstanceOperationType,
		instanceID string,
		comment string,
		author string,
	) (string, error)
	GetInstanceOperation(ctx context.Context, operationID string) (models.ManagementInstanceOperation, error)
	InstanceOperationsToProcess(ctx context.Context, limit int) ([]models.ManagementInstanceOperation, error)
	InstanceOperationsToAlarm(ctx context.Context) ([]models.ManagementInstanceOperation, error)
	UpdateInstanceOperationFields(ctx context.Context, operation models.ManagementInstanceOperation) error
	StaleInstanceOperations(ctx context.Context) ([]models.ManagementInstanceOperation, error)
	ListInstanceOperations(ctx context.Context) ([]models.ManagementInstanceOperation, error)
}

type LockKey int64

var (
	Dom0AutodutyLockKey     LockKey = 1000
	InstanceAutodutyLockKey LockKey = 2000
)
