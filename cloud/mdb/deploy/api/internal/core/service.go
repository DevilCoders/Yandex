package core

import (
	"bytes"
	"context"
	"net/http"
	"regexp"
	"strings"
	"sync/atomic"
	"time"

	"github.com/opentracing/opentracing-go"
	"github.com/opentracing/opentracing-go/ext"

	"a.yandex-team.ru/cloud/mdb/deploy/api/internal/deploydb"
	"a.yandex-team.ru/cloud/mdb/deploy/api/pkg/models"
	"a.yandex-team.ru/cloud/mdb/deploy/internal/tracing/tags"
	"a.yandex-team.ru/cloud/mdb/internal/auth/blackbox/restapi"
	"a.yandex-team.ru/cloud/mdb/internal/auth/httpauth"
	"a.yandex-team.ru/cloud/mdb/internal/auth/httpauth/blackboxauth"
	"a.yandex-team.ru/cloud/mdb/internal/auth/httpauth/combinedauth"
	"a.yandex-team.ru/cloud/mdb/internal/auth/httpauth/iamauth"
	"a.yandex-team.ru/cloud/mdb/internal/auth/tvm/tvmtool"
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/saltapi"
	"a.yandex-team.ru/cloud/mdb/internal/saltapi/cherrypy"
	salttags "a.yandex-team.ru/cloud/mdb/internal/saltapi/tracing/tags"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

// Known service auth errors
var (
	ErrAuthTokenEmpty       = xerrors.NewSentinel("empty auth token")
	ErrAuthTemporaryFailure = xerrors.NewSentinel("failed to authenticate (temporary error)")
	ErrAuthFailure          = xerrors.NewSentinel("failed to authenticate")
	ErrAuthNoRights         = xerrors.NewSentinel("action not authorized")
)

type AuthConfig struct {
	BlackBoxEnabled        bool                `json:"blackbox_enabled" yaml:"blackbox_enabled"`
	BlackBox               blackboxauth.Config `json:"blackbox" yaml:"blackbox"`
	IAMEnabled             bool                `json:"iam_enabled" yaml:"iam_enabled"`
	IAM                    iamauth.Config      `json:"iam" yaml:"iam"`
	NotForProductionNoAuth bool                `json:"not_for_production_no_auth" yaml:"not_for_production_no_auth"`
}

// Config describes configuration for mdb-deploy-api
type Config struct {
	SaltAPI                      cherrypy.ClientConfig `json:"saltapi" yaml:"saltapi"`
	RestrictToLocalIdentifier    string                `json:"restrict_to_local_identifier" yaml:"restrict_to_local_identifier"`
	MasterCheckerName            string                `json:"master_checker_name" yaml:"master_checker_name"`
	MasterCheckPeriod            time.Duration         `json:"master_check_period" yaml:"master_check_period"`
	MasterCheckFailCount         int                   `json:"master_check_fail_count" yaml:"master_check_fail_count"`
	ShipmentTimeoutPeriod        time.Duration         `json:"shipment_timeout_period" yaml:"shipment_timeout_period"`
	ShipmentTimeoutPageSize      int64                 `json:"shipment_timeout_pagesize" yaml:"shipment_timeout_pagesize"`
	CommandDispatchPeriod        time.Duration         `json:"command_dispatch_period" yaml:"command_dispatch_period"`
	CommandTimeoutPeriod         time.Duration         `json:"command_timeout_period" yaml:"command_timeout_period"`
	CommandTimeoutPageSize       int64                 `json:"command_timeout_pagesize" yaml:"command_timeout_pagesize"`
	RunningJobCheck              RunningJobCheckConfig `json:"running_job_check" yaml:"running_job_check"`
	SyncMastersListPeriod        time.Duration         `json:"sync_masters_list_period" yaml:"sync_masters_list_period"`
	SyncMastersListPageSize      int64                 `json:"sync_masters_list_pagesize" yaml:"sync_masters_list_pagesize"`
	FailoverMinionsPeriod        time.Duration         `json:"failover_minions_period" yaml:"failover_minions_period"`
	FailoverMinionsSize          int64                 `json:"failover_minions_size" yaml:"failover_minions_size"`
	JobResultBlacklistPath       string                `json:"job_result_blacklist_path" yaml:"job_result_blacklist_path"`
	Auth                         AuthConfig            `json:"auth" yaml:"auth"`
	MastersTimeZoneOffset        time.Duration         `json:"salt_master_tz_offset" yaml:"salt_master_tz_offset"`
	SaltAPITimeouts              SaltAPITimeoutsConfig `json:"salt_api_timeouts" yaml:"salt_api_timeouts"`
	MasterDefaultPublicKey       string                `json:"master_default_public_key" yaml:"master_default_public_key"`
	MasterMaxTasksPerThread      int32                 `json:"master_max_tasks_per_thread" yaml:"master_max_tasks_per_thread"`
	AllowDeletedMinionRecreation bool                  `json:"allow_deleted_minion_recreation" yaml:"allow_deleted_minion_recreation"`
	SkipShipmentsFQDNRegexes     []string              `json:"skip_shipments_fqdn_regexes" yaml:"skip_shipments_fqdn_regexes"`
}

// DefaultConfig returns default config for service
func DefaultConfig() Config {
	cfg := Config{
		SaltAPI:                 cherrypy.DefaultClientConfig(),
		MasterCheckPeriod:       time.Second * 5,
		MasterCheckFailCount:    3,
		ShipmentTimeoutPeriod:   time.Second * 30,
		ShipmentTimeoutPageSize: 100,
		CommandDispatchPeriod:   time.Second * 5,
		CommandTimeoutPeriod:    time.Second * 5,
		CommandTimeoutPageSize:  100,
		RunningJobCheck:         DefaultRunningJobCheckConfig(),
		SyncMastersListPeriod:   time.Second * 5,
		SyncMastersListPageSize: 1000,
		FailoverMinionsPeriod:   time.Second * 15,
		FailoverMinionsSize:     10,
		JobResultBlacklistPath:  "/etc/yandex/mdb-deploy/job_result_blacklist.yaml",
		Auth: AuthConfig{
			BlackBoxEnabled: true,
			BlackBox: blackboxauth.Config{
				Tvm: tvmtool.Config{
					Alias: "mdb-deploy-api",
					Token: "",
					URI:   "http://localhost:50001",
				},
				BlackboxURI:   restapi.IntranetURL,
				BlackboxAlias: "blackbox",
			},
			IAMEnabled: true,
			IAM:        iamauth.DefaultConfig(),
		},
		MastersTimeZoneOffset:        3 * time.Hour, // Moscow
		SaltAPITimeouts:              DefaultSaltAPITimeoutsConfig(),
		MasterMaxTasksPerThread:      1,
		AllowDeletedMinionRecreation: false,
	}

	return cfg
}

type RunningJobCheckConfig struct {
	Period          time.Duration `json:"period" yaml:"period"`
	RunningInterval time.Duration `json:"running_interval" yaml:"running_interval"`
	PageSize        int64         `json:"pagesize" yaml:"pagesize"`
	FailOnCount     int           `json:"fail_on_count" yaml:"fail_on_count"`
}

func DefaultRunningJobCheckConfig() RunningJobCheckConfig {
	return RunningJobCheckConfig{
		Period:          time.Second * 5,
		RunningInterval: time.Second * 30,
		PageSize:        5,
		FailOnCount:     3,
	}
}

type JobResultBlacklist struct {
	Blacklist []BlacklistedJobResult `yaml:"blacklist"`
}

func DefaultJobResultBlacklist() JobResultBlacklist {
	return JobResultBlacklist{}
}

func (b JobResultBlacklist) IsBlacklisted(f string, args []string) bool {
	for _, b := range b.Blacklist {
		if b.Matches(f, args) {
			return true
		}
	}

	return false
}

type BlacklistedJobResult struct {
	Func string   `yaml:"func,omitempty"`
	Args []string `yaml:"args,omitempty"`
}

func (bjr BlacklistedJobResult) Matches(f string, args []string) bool {
	// If func is supplied and it doesn't match, fail
	if bjr.Func != "" && f != bjr.Func {
		return false
	}

	// If args are empty, success
	if len(bjr.Args) == 0 {
		return true
	}

	// Match args
	for _, a := range bjr.Args {
		var found bool
		for _, b := range args {
			if a == b {
				found = true
				break
			}
		}

		if !found {
			return false
		}
	}

	return true
}

func localityIdentifierFromFQDN(fqdn string) (string, string, error) {
	i := strings.Index(fqdn, ".")
	if i == -1 {
		return "", "", xerrors.New("not fqdn")
	}

	li := fqdn[i+1:]
	if li == "" {
		return "", "", xerrors.New("empty locality identifier")
	}

	return fqdn[:i], li, nil
}

// Service implements mdb-deploy-api logic
type Service struct {
	cfg                Config
	jobResultBlacklist JobResultBlacklist
	lg                 log.Logger

	ddb     deploydb.Backend
	auth    httpauth.Authenticator
	masters atomic.Value
}

const (
	minMasterCheckPeriod = time.Second * 5
)

type notForProductionNoAuth struct{}

func newNotForProductionNoAuth() *notForProductionNoAuth {
	return &notForProductionNoAuth{}
}

func (notForProductionNoAuth) Ping(_ context.Context) error {
	return nil
}

func (notForProductionNoAuth) Auth(_ context.Context, _ *http.Request) error {
	return nil
}

// NewService constructs Service
func NewService(ctx context.Context, b deploydb.Backend, cfg Config, jrBlacklist JobResultBlacklist, lg log.Logger) (*Service, error) {
	if cfg.MasterCheckPeriod < minMasterCheckPeriod {
		lg.Warnf("Master check period is less than %s, setting it to that value", minMasterCheckPeriod)
		cfg.MasterCheckPeriod = minMasterCheckPeriod
	}

	var authProviders []httpauth.Authenticator

	if cfg.Auth.IAMEnabled {
		iamAuth, err := iamauth.New(ctx, cfg.Auth.IAM, lg)
		if err != nil {
			return nil, err
		}
		authProviders = append(authProviders, iamAuth)
	}

	if cfg.Auth.BlackBoxEnabled {
		bbAuth, err := blackboxauth.NewFromConfig(cfg.Auth.BlackBox, lg)
		if err != nil {
			return nil, err
		}
		authProviders = append(authProviders, bbAuth)
	}

	if cfg.Auth.NotForProductionNoAuth {
		authProviders = append(authProviders, newNotForProductionNoAuth())
	}

	auth, err := combinedauth.New(lg, authProviders...)
	if err != nil {
		return nil, err
	}

	srv := &Service{
		ddb:                b,
		cfg:                cfg,
		jobResultBlacklist: jrBlacklist,
		auth:               auth,
		lg:                 lg,
	}

	if cfg.RestrictToLocalIdentifier != "" {
		lg.Infof("Using %q as locality identifier", srv.cfg.RestrictToLocalIdentifier)
	} else {
		lg.Info("No locality identifier set, not restricting to local installation")
	}

	if cfg.MasterDefaultPublicKey == "" {
		return nil, xerrors.New("master_default_public_key cannot be empty")
	}

	// Init masters list
	srv.masters.Store(map[string]*knownMaster{})

	go srv.backgroundMastersListSync(ctx)
	go srv.backgroundShipmentsTimeout(ctx)
	go srv.backgroundJobsTimeout(ctx)
	go srv.backgroundFailoverMinions(ctx)
	return srv, nil
}

// IsReady checks is service is ready to serve requests
func (srv *Service) IsReady(ctx context.Context) error {
	if err := srv.ddb.IsReady(ctx); err != nil {
		srv.lg.Warnf("deploy db is not ready: %s", err)
		return err
	}

	if err := srv.auth.Ping(ctx); err != nil {
		srv.lg.Warnf("auth subsystem is not ready: %s", err)
		return err
	}

	return nil
}

// CreateGroup creates group
func (srv *Service) CreateGroup(ctx context.Context, name string) (models.Group, error) {
	return srv.ddb.CreateGroup(ctx, name)
}

// Group returns group
func (srv *Service) Group(ctx context.Context, name string) (models.Group, error) {
	return srv.ddb.Group(ctx, name)
}

// Groups returns groups
func (srv *Service) Groups(ctx context.Context, sortOrder models.SortOrder, limit int64, lastGroupID optional.Int64) ([]models.Group, error) {
	return srv.ddb.Groups(ctx, sortOrder, limit, lastGroupID)
}

// CreateMaster creates master
func (srv *Service) CreateMaster(ctx context.Context, fqdn, group string, isOpen bool, desc string) (models.Master, error) {
	return srv.ddb.CreateMaster(ctx, fqdn, group, isOpen, desc)
}

// UpsertMaster creates new master or updates the old one
func (srv *Service) UpsertMaster(ctx context.Context, fqdn string, attrs deploydb.UpsertMasterAttrs) (models.Master, error) {
	return srv.ddb.UpsertMaster(ctx, fqdn, attrs)
}

// Master returns master
func (srv *Service) Master(ctx context.Context, fqdn string) (models.Master, error) {
	master, err := srv.ddb.Master(ctx, fqdn)
	if err != nil {
		return models.Master{}, err
	}

	master.PublicKey = srv.cfg.MasterDefaultPublicKey

	return master, nil
}

// Masters returns masters
func (srv *Service) Masters(ctx context.Context, limit int64, lastMasterID optional.Int64) ([]models.Master, error) {
	return srv.ddb.Masters(ctx, limit, lastMasterID)
}

// MinionsByMaster returns minions of specific master
func (srv *Service) MinionsByMaster(ctx context.Context, fqdn string, limit int64, lastMinionID optional.Int64) ([]models.Minion, error) {
	return srv.ddb.MinionsByMaster(ctx, fqdn, limit, lastMinionID)
}

// CreateMinion creates minion
func (srv *Service) CreateMinion(ctx context.Context, fqdn, group string, autoReassign bool) (models.Minion, error) {
	return srv.ddb.CreateMinion(ctx, fqdn, group, autoReassign)
}

// UpsertMinion creates new minion or updates the old one
func (srv *Service) UpsertMinion(ctx context.Context, fqdn string, attrs deploydb.UpsertMinionAttrs) (models.Minion, error) {
	return srv.ddb.UpsertMinion(ctx, fqdn, attrs, srv.cfg.AllowDeletedMinionRecreation)
}

// Minion returns minion
func (srv *Service) Minion(ctx context.Context, fqdn string) (models.Minion, error) {
	return srv.ddb.Minion(ctx, fqdn)
}

// Minions returns minions
func (srv *Service) Minions(ctx context.Context, limit int64, lastMinionID optional.Int64) ([]models.Minion, error) {
	return srv.ddb.Minions(ctx, limit, lastMinionID)
}

// RegisterMinion registers minion's public key
func (srv *Service) RegisterMinion(ctx context.Context, fqdn string, pubKey string) (models.Minion, error) {
	return srv.ddb.RegisterMinion(ctx, fqdn, pubKey)
}

// UnregisterMinion removes minion's public key and resets registration timeout allowing for new registration
func (srv *Service) UnregisterMinion(ctx context.Context, fqdn string) (models.Minion, error) {
	return srv.ddb.UnregisterMinion(ctx, fqdn)
}

// DeleteMinion by fqdn
func (srv *Service) DeleteMinion(ctx context.Context, fqdn string) error {
	return srv.ddb.DeleteMinion(ctx, fqdn)
}

// CreateShipment creates shipment
func (srv *Service) CreateShipment(
	ctx context.Context,
	fqdns []string,
	commands []models.CommandDef,
	parallel, stopOnErrorCount int64,
	timeout time.Duration,
) (models.Shipment, error) {
	span, ctx := opentracing.StartSpanFromContext(ctx, "Deploy Create Shipment")
	defer span.Finish()

	carrier := opentracing.TextMapCarrier{}
	if err := opentracing.GlobalTracer().Inject(
		span.Context(),
		opentracing.TextMap,
		carrier,
	); err != nil {
		srv.lg.Warn("failed to prepare tracing carrier for shipment", log.Error(err))
	}

	skipFQDNs := make([]string, 0, len(fqdns))
	for _, fqdn := range fqdns {
		for _, regex := range srv.cfg.SkipShipmentsFQDNRegexes {
			matched, err := regexp.MatchString(regex, fqdn)
			if err != nil {
				return models.Shipment{}, err
			}
			if matched {
				skipFQDNs = append(skipFQDNs, fqdn)
				break
			}
		}
	}

	shipment, err := srv.ddb.CreateShipment(ctx, fqdns, skipFQDNs, commands, parallel, stopOnErrorCount, timeout, carrier)
	if err != nil {
		return models.Shipment{}, err
	}

	tags.ShipmentID.Set(span, shipment.ID)
	return shipment, nil
}

// Shipment returns shipment
func (srv *Service) Shipment(ctx context.Context, id models.ShipmentID) (models.Shipment, error) {
	return srv.ddb.Shipment(ctx, id)
}

// Shipments returns shipments
func (srv *Service) Shipments(ctx context.Context, attrs deploydb.SelectShipmentsAttrs, limit int64, lastShipmentID optional.Int64) ([]models.Shipment, error) {
	return srv.ddb.Shipments(ctx, attrs, limit, lastShipmentID)
}

// Command returns command
func (srv *Service) Command(ctx context.Context, id models.CommandID) (models.Command, error) {
	return srv.ddb.Command(ctx, id)
}

// Commands returns commands
func (srv *Service) Commands(ctx context.Context, attrs deploydb.SelectCommandsAttrs, limit int64, lastCommandID optional.Int64) ([]models.Command, error) {
	return srv.ddb.Commands(ctx, attrs, limit, lastCommandID)
}

// Job returns job
func (srv *Service) Job(ctx context.Context, id models.JobID) (models.Job, error) {
	return srv.ddb.Job(ctx, id)
}

// Jobs returns jobs
func (srv *Service) Jobs(ctx context.Context, attrs deploydb.SelectJobsAttrs, limit int64, lastJobID optional.Int64) ([]models.Job, error) {
	return srv.ddb.Jobs(ctx, attrs, limit, lastJobID)
}

// EscapeUnicodeNULL replace Unicode NULL with 'NULL symbol' in JSON string
// Postgre doesn't allow string with \0 (eg NULL).
// ERROR:  unsupported Unicode escape sequence
// ...
//		DETAIL:  \u0000 cannot be converted to text
//
func EscapeUnicodeNULL(v []byte) []byte {
	// Looks like there is one way to write Unicode NULL in json strings.
	// And it is '\u0000'.
	// json string grammar
	//
	//	string
	//		'"' characters '"'
	//
	//	characters
	//		""
	//		character characters
	//
	//	character
	//		'0020' . '10ffff' - '"' - '\'
	//		'\' escape
	//
	//	escape
	//		'"'
	//		'\'
	//		'/'
	//		'b'
	//		'f'
	//		'n'
	//		'r'
	//		't'
	//		'u' hex hex hex hex

	// Symbols escaped, cause we fixing JSON
	return bytes.ReplaceAll(v, []byte("\\u0000"), []byte("\\u2400"))
}

// CreateJobResult creates job result
func (srv *Service) CreateJobResult(ctx context.Context, jobExtID, fqdn string, result []byte) (models.JobResult, error) {
	status := models.JobResultStatusUnknown
	var coords models.JobResultCoords
	rr, errs := saltapi.ParseReturnResult(result, srv.cfg.MastersTimeZoneOffset)
	// Log what we parsed
	switch rr.ParseLevel {
	case saltapi.ParseLevelNone:
		srv.lg.Errorf("failed to parse job result id %q and fqdn %q: %s", jobExtID, fqdn, errs)
	case saltapi.ParseLevelMinimal:
		srv.lg.Warnf("job result id %q for fqdn %q was minimally parsed: %s", jobExtID, fqdn, errs)
	case saltapi.ParseLevelPartial:
		srv.lg.Warnf("job result id %q for fqdn %q was partially parsed: %s", jobExtID, fqdn, errs)
	case saltapi.ParseLevelFull:
		if len(errs) != 0 {
			srv.lg.Warnf("job result id %q for fqdn %q was fully parsed but has errors: %s", jobExtID, fqdn, errs)
		}
	}

	// We can check blacklist and trace job result only if we parsed something
	if rr.ParseLevel != saltapi.ParseLevelNone {
		// Do not handle blacklisted job results in any way
		if srv.jobResultBlacklist.IsBlacklisted(rr.Func, rr.FuncArgs) {
			srv.lg.Debugf("job result id %q and fqdn %q matches blacklist, not storing it", jobExtID, fqdn)
			return models.JobResult{}, nil
		}

		// Coords will be filled later
		defer func() { srv.traceJobResult(coords, jobExtID, fqdn, rr) }()
	}

	// Only partial and full parses are eligible for success check
	if rr.CanCheckSuccess() {
		if rr.IsSuccessful() {
			status = models.JobResultStatusSuccess
		} else {
			status = models.JobResultStatusFailure
		}
	}

	// It might be tempting to log result in case we actually escaped something but it is unwise since
	// results can be HUGE
	safeResult := EscapeUnicodeNULL(result)

	ctx, err := srv.ddb.Begin(ctx, sqlutil.Primary)
	if err != nil {
		return models.JobResult{}, err
	}
	defer func() { _ = srv.ddb.Rollback(ctx) }()

	jr, err := srv.ddb.CreateJobResult(ctx, jobExtID, fqdn, status, safeResult)
	if err != nil {
		return models.JobResult{}, err
	}

	// Store in coords taht will be used in tracing defer
	coords, err = srv.ddb.JobResultCoords(ctx, jobExtID, fqdn)
	if err != nil {
		srv.lg.Warnf("job result id %q for fqdn %q (job id %d) is not bound to any known job: %s", jobExtID, fqdn, jr.JobResultID, err)
	}

	return jr, srv.ddb.Commit(ctx)
}

// traceJobResult accepts jid and fqdn separately because this way we can add tracing for unparsed results
func (srv *Service) traceJobResult(coords models.JobResultCoords, jid, fqdn string, rr saltapi.ReturnResult) {
	spanOpts := []opentracing.StartSpanOption{
		opentracing.StartTime(rr.StartTS),
		// Cast custom types to int, otherwise they will be treated as 'strings'
		tags.ShipmentID.Tag(coords.ShipmentID),
		tags.CommandID.Tag(coords.CommandID),
		tags.JobID.Tag(coords.JobID),
		tags.JobExtID.Tag(jid),
		tags.MinionFQDN.Tag(fqdn),
		salttags.JobFunc.Tag(rr.Func),
		// Function args might contain secrets so they are NOT safe to add to traces
		salttags.JobStatesCount.Tag(len(rr.ReturnStates)),
	}

	if coords.Tracing != nil {
		parentSpan, err := opentracing.GlobalTracer().Extract(opentracing.TextMap, coords.Tracing)
		if err != nil {
			srv.lg.Warn(
				"can't extract opentracing span context",
				log.Reflect("opentracing_carrier_format", opentracing.TextMap),
				log.Reflect("opentracing_carrier", coords.Tracing),
				log.Error(err),
			)
		} else {
			spanOpts = append(spanOpts, opentracing.FollowsFrom(parentSpan))
		}
	}

	jobSpan := opentracing.StartSpan("SaltStack Job", spanOpts...)
	defer jobSpan.FinishWithOptions(opentracing.FinishOptions{FinishTime: rr.FinishTS})

	// Jobs with test=True can have spurious errors so do not mark them in traces
	testRun := rr.IsTestRun()
	if testRun {
		salttags.SaltTest.Set(jobSpan, true)
	}

	if !testRun && rr.CanCheckSuccess() && !rr.IsSuccessful() {
		ext.Error.Set(jobSpan, true)
	}

	for _, v := range rr.ReturnStates {
		if v == nil {
			continue
		}

		// Handle zero times
		if v.StartTS.IsZero() {
			v.StartTS = rr.StartTS
		}
		if v.FinishTS.IsZero() {
			v.FinishTS = v.StartTS
		}

		span := opentracing.StartSpan(
			"SaltStack State",
			salttags.StateSLS.Tag(v.SLS),
			salttags.StateID.Tag(v.ID),
			salttags.StateStartTime.Tag(v.StartTime),
			salttags.StateDuration.Tag(v.Duration.Duration),
			salttags.StateResult.Tag(v.Result),
			salttags.StateRunNum.Tag(v.RunNum),
			opentracing.StartTime(v.StartTS),
			opentracing.ChildOf(jobSpan.Context()),
		)

		if !testRun && !v.Result {
			ext.Error.Set(span, true)
		}

		span.FinishWithOptions(opentracing.FinishOptions{FinishTime: v.FinishTS})
	}
}

// JobResult returns job result with specified id
func (srv *Service) JobResult(ctx context.Context, id models.JobResultID) (models.JobResult, error) {
	return srv.ddb.JobResult(ctx, id)
}

// JobResults returns job results for specific job id and fqdn
func (srv *Service) JobResults(ctx context.Context, attrs deploydb.SelectJobResultsAttrs, limit int64, lastJobResultID optional.Int64) ([]models.JobResult, error) {
	return srv.ddb.JobResults(ctx, attrs, limit, lastJobResultID)
}

// Auth request
func (srv *Service) Auth(ctx context.Context, r *http.Request) error {
	err := srv.auth.Auth(ctx, r)
	switch {
	case xerrors.Is(err, httpauth.ErrAuthFailure):
		return ErrAuthFailure.Wrap(err)
	case xerrors.Is(err, httpauth.ErrAuthNoRights):
		return ErrAuthNoRights.Wrap(err)
	case xerrors.Is(err, httpauth.ErrNoAuthCredentials):
		return ErrAuthTokenEmpty.Wrap(err)
	case err != nil:
		return ErrAuthTemporaryFailure.Wrap(err)
	}
	return nil
}
