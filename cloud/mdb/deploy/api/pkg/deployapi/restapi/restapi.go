package restapi

import (
	"context"
	"net/url"
	"time"

	swagclient "a.yandex-team.ru/cloud/mdb/deploy/api/generated/swagger/client"
	swagcmds "a.yandex-team.ru/cloud/mdb/deploy/api/generated/swagger/client/commands"
	swagcommon "a.yandex-team.ru/cloud/mdb/deploy/api/generated/swagger/client/common"
	swaggroups "a.yandex-team.ru/cloud/mdb/deploy/api/generated/swagger/client/groups"
	swagmasters "a.yandex-team.ru/cloud/mdb/deploy/api/generated/swagger/client/masters"
	swagminions "a.yandex-team.ru/cloud/mdb/deploy/api/generated/swagger/client/minions"
	swagmodels "a.yandex-team.ru/cloud/mdb/deploy/api/generated/swagger/models"
	"a.yandex-team.ru/cloud/mdb/deploy/api/internal/api/swagapi"
	"a.yandex-team.ru/cloud/mdb/deploy/api/pkg/deployapi"
	"a.yandex-team.ru/cloud/mdb/deploy/api/pkg/deployapi/deployutils"
	"a.yandex-team.ru/cloud/mdb/deploy/api/pkg/models"
	"a.yandex-team.ru/cloud/mdb/internal/auth/tvm"
	"a.yandex-team.ru/cloud/mdb/internal/compute/iam"
	"a.yandex-team.ru/cloud/mdb/internal/httputil"
	"a.yandex-team.ru/cloud/mdb/internal/httputil/openapi"
	"a.yandex-team.ru/cloud/mdb/internal/requestid"
	"a.yandex-team.ru/cloud/mdb/internal/secret"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/ptr"
)

type client struct {
	logger  log.Logger
	cli     *swagclient.MdbDeployapi
	token   string
	iamCred iam.CredentialsService
}

var _ deployapi.Client = &client{}

const (
	iamTokenPrefix = "Bearer "
)

// TransportConfig ...
type TransportConfig struct {
	TLS     httputil.TLSConfig     `json:"tls" yaml:"tls"`
	Logging httputil.LoggingConfig `json:"logging" yaml:"logging"`
}

// Config is deploy client config
type Config struct {
	URI       string          `json:"uri" yaml:"uri"`
	Token     secret.String   `json:"token" yaml:"token"`
	Transport TransportConfig `json:"transport" yaml:"transport"`
}

func DefaultConfig() Config {
	return Config{}
}

// New constructs new mdb-deploy-api client
func New(uri, token string, iamCred iam.CredentialsService, tlscfg httputil.TLSConfig, logcfg httputil.LoggingConfig, l log.Logger) (deployapi.Client, error) {
	if uri == "" {
		l.Debug("Provided MDB Deploy API url is empty. Deducing...")
		var err error
		uri, err = deployutils.DeployAPIURL()
		if err != nil {
			return nil, xerrors.Errorf("MDB Deploy API url is empty and we failed to deduce it: %w", err)
		}
	}

	u, err := url.Parse(uri)
	if err != nil {
		return nil, err
	}

	rt, err := httputil.DEPRECATEDNewTransport(tlscfg, logcfg, l)
	if err != nil {
		return nil, err
	}

	crt := openapi.NewRuntime(
		u.Host,
		swagclient.DefaultBasePath,
		[]string{u.Scheme},
		rt,
		l,
	)

	return &client{
		logger:  l,
		cli:     swagclient.New(crt, nil),
		token:   token,
		iamCred: iamCred,
	}, nil
}

// NewFromConfig constructs new mdb-deploy-api client from config
func NewFromConfig(config Config, l log.Logger) (deployapi.Client, error) {
	return New(config.URI, config.Token.Unmask(), nil, config.Transport.TLS, config.Transport.Logging, l)
}

func (c *client) IsReady(ctx context.Context) error {
	_, err := c.cli.Common.Ping(swagcommon.NewPingParamsWithContext(ctx))
	if err == nil {
		return nil
	}

	return handleError(err)
}

func (c *client) getToken(ctx context.Context) *string {
	if c.iamCred == nil {
		return ptr.String(tvm.FormatOAuthToken(c.token))
	}
	token, err := c.iamCred.Token(ctx)
	if err != nil {
		c.logger.Errorf("failed to request IAM token: %s", err)
		return nil
	}
	return ptr.String(iamTokenPrefix + token)
}

func (c *client) CreateGroup(ctx context.Context, name string) (models.Group, error) {
	req := swaggroups.NewCreateGroupParamsWithContext(ctx)
	req = req.WithAuthorization(c.getToken(req.Context))
	req = req.WithBody(&swagmodels.Group{Name: name})
	req = req.WithXRequestID(ptr.String(requestid.FromContextOrNew(ctx)))

	resp, err := c.cli.Groups.CreateGroup(req)
	if err != nil {
		return models.Group{}, handleError(err)
	}

	return groupFromREST(resp.Payload), nil
}

func (c *client) GetGroup(ctx context.Context, name string) (models.Group, error) {
	req := swaggroups.NewGetGroupParamsWithContext(ctx)
	req = req.WithAuthorization(c.getToken(req.Context))
	req = req.WithGroupname(name)
	req = req.WithXRequestID(ptr.String(requestid.FromContextOrNew(ctx)))

	resp, err := c.cli.Groups.GetGroup(req)
	if err != nil {
		return models.Group{}, handleError(err)
	}

	return groupFromREST(resp.Payload), nil
}

// GetGroups retrieves list of known groups
func (c *client) GetGroups(ctx context.Context, paging deployapi.Paging) ([]models.Group, deployapi.Paging, error) {
	req := swaggroups.NewGetGroupsListParamsWithContext(ctx)
	req = req.WithAuthorization(c.getToken(req.Context))

	if paging.HasSortOrder() {
		so, err := swagapi.SortOrderToREST(paging.SortOrder)
		if err != nil {
			return nil, deployapi.Paging{}, err
		}
		req = req.WithSortOrder(&so)
	}
	if paging.Size != 0 {
		req = req.WithPageSize(&paging.Size)
	}
	if paging.Token != "" {
		req = req.WithPageToken(&paging.Token)
	}
	req = req.WithXRequestID(ptr.String(requestid.FromContextOrNew(ctx)))

	resp, err := c.cli.Groups.GetGroupsList(req)
	if err != nil {
		return nil, deployapi.Paging{}, handleError(err)
	}

	var groups []models.Group
	for _, group := range resp.Payload.Groups {
		groups = append(groups, groupFromREST(group))
	}

	paging.Token = resp.Payload.Paging.Token
	return groups, paging, nil
}

func (c *client) CreateMaster(ctx context.Context, fqdn, group string, isOpen bool, desc string) (models.Master, error) {
	req := swagmasters.NewCreateMasterParamsWithContext(ctx)
	req = req.WithAuthorization(c.getToken(req.Context))
	req = req.WithBody(&swagmodels.Master{Fqdn: fqdn, Group: &group, IsOpen: &isOpen, Description: &desc})
	req = req.WithXRequestID(ptr.String(requestid.FromContextOrNew(ctx)))

	resp, err := c.cli.Masters.CreateMaster(req)
	if err != nil {
		return models.Master{}, handleError(err)
	}

	return masterFromREST(resp.Payload), nil
}

func (c *client) UpsertMaster(ctx context.Context, fqdn string, attrs deployapi.UpsertMasterAttrs) (models.Master, error) {
	req := swagmasters.NewUpsertMasterParamsWithContext(ctx)
	req = req.WithAuthorization(c.getToken(req.Context))
	req = req.WithFqdn(fqdn)

	m := swagmodels.Master{Fqdn: fqdn}
	if attrs.Group.Valid {
		m.Group = &attrs.Group.String
	}
	if attrs.IsOpen.Valid {
		m.IsOpen = &attrs.IsOpen.Bool
	}
	if attrs.Description.Valid {
		m.Description = &attrs.Description.String
	}
	req = req.WithBody(&m)
	req = req.WithXRequestID(ptr.String(requestid.FromContextOrNew(ctx)))

	resp, err := c.cli.Masters.UpsertMaster(req)
	if err != nil {
		return models.Master{}, handleError(err)
	}

	return masterFromREST(resp.Payload), nil
}

func (c *client) GetMaster(ctx context.Context, fqdn string) (models.Master, error) {
	req := swagmasters.NewGetMasterParamsWithContext(ctx)
	req = req.WithAuthorization(c.getToken(req.Context))
	req = req.WithFqdn(fqdn)
	req = req.WithXRequestID(ptr.String(requestid.FromContextOrNew(ctx)))

	resp, err := c.cli.Masters.GetMaster(req)
	if err != nil {
		return models.Master{}, handleError(err)
	}

	return masterFromREST(resp.Payload), nil
}

func (c *client) GetMasters(ctx context.Context, paging deployapi.Paging) ([]models.Master, deployapi.Paging, error) {
	req := swagmasters.NewGetMastersListParamsWithContext(ctx)
	req = req.WithAuthorization(c.getToken(req.Context))
	if paging.Size != 0 {
		req = req.WithPageSize(&paging.Size)
	}
	if paging.Token != "" {
		req = req.WithPageToken(&paging.Token)
	}
	req = req.WithXRequestID(ptr.String(requestid.FromContextOrNew(ctx)))

	resp, err := c.cli.Masters.GetMastersList(req)
	if err != nil {
		return nil, deployapi.Paging{}, handleError(err)
	}

	var masters []models.Master
	for _, master := range resp.Payload.Masters {
		masters = append(masters, masterFromREST(master))
	}

	paging.Token = resp.Payload.Paging.Token
	return masters, paging, nil
}

func (c *client) CreateMinion(ctx context.Context, fqdn, group string, autoReassign bool) (models.Minion, error) {
	req := swagminions.NewCreateMinionParamsWithContext(ctx)
	req = req.WithAuthorization(c.getToken(req.Context))
	req = req.WithBody(&swagmodels.Minion{Fqdn: fqdn, Group: &group, AutoReassign: &autoReassign})
	req = req.WithXRequestID(ptr.String(requestid.FromContextOrNew(ctx)))

	resp, err := c.cli.Minions.CreateMinion(req)
	if err != nil {
		return models.Minion{}, handleError(err)
	}

	return minionFromREST(resp.Payload), nil
}

func (c *client) UpsertMinion(ctx context.Context, fqdn string, attrs deployapi.UpsertMinionAttrs) (models.Minion, error) {
	req := swagminions.NewUpsertMinionParamsWithContext(ctx)
	req = req.WithAuthorization(c.getToken(req.Context))
	req = req.WithFqdn(fqdn)

	m := swagmodels.Minion{Fqdn: fqdn}
	if attrs.Group.Valid {
		m.Group = &attrs.Group.String
	}
	if attrs.AutoReassign.Valid {
		m.AutoReassign = &attrs.AutoReassign.Bool
	}
	if attrs.Master.Valid {
		m.Master = &attrs.Master.String
	}

	req = req.WithBody(&m)
	req = req.WithXRequestID(ptr.String(requestid.FromContextOrNew(ctx)))

	resp, err := c.cli.Minions.UpsertMinion(req)
	if err != nil {
		return models.Minion{}, handleError(err)
	}

	return minionFromREST(resp.Payload), nil
}

func (c *client) GetMinion(ctx context.Context, fqdn string) (models.Minion, error) {
	req := swagminions.NewGetMinionParamsWithContext(ctx)
	req = req.WithAuthorization(c.getToken(req.Context))
	req = req.WithFqdn(fqdn)
	req = req.WithXRequestID(ptr.String(requestid.FromContextOrNew(ctx)))

	resp, err := c.cli.Minions.GetMinion(req)
	if err != nil {
		return models.Minion{}, handleError(err)
	}

	return minionFromREST(resp.Payload), nil
}

func (c *client) GetMinions(ctx context.Context, paging deployapi.Paging) ([]models.Minion, deployapi.Paging, error) {
	req := swagminions.NewGetMinionsListParamsWithContext(ctx)
	req = req.WithAuthorization(c.getToken(req.Context))
	if paging.Size != 0 {
		req = req.WithPageSize(&paging.Size)
	}
	if paging.Token != "" {
		req = req.WithPageToken(&paging.Token)
	}
	req = req.WithXRequestID(ptr.String(requestid.FromContextOrNew(ctx)))

	resp, err := c.cli.Minions.GetMinionsList(req)
	if err != nil {
		return nil, deployapi.Paging{}, handleError(err)
	}

	var minions []models.Minion
	for _, minion := range resp.Payload.Minions {
		minions = append(minions, minionFromREST(minion))
	}

	paging.Token = resp.Payload.Paging.Token
	return minions, paging, nil
}

func (c *client) GetMinionsByMaster(ctx context.Context, fqdn string, paging deployapi.Paging) ([]models.Minion, deployapi.Paging, error) {
	req := swagmasters.NewGetMasterMinionsParamsWithContext(ctx)
	req = req.WithAuthorization(c.getToken(req.Context))
	req = req.WithFqdn(fqdn)
	if paging.Size != 0 {
		req = req.WithPageSize(&paging.Size)
	}
	if paging.Token != "" {
		req = req.WithPageToken(&paging.Token)
	}
	req = req.WithXRequestID(ptr.String(requestid.FromContextOrNew(ctx)))

	resp, err := c.cli.Masters.GetMasterMinions(req)
	if err != nil {
		return nil, deployapi.Paging{}, handleError(err)
	}

	var minions []models.Minion
	for _, minion := range resp.Payload.Minions {
		minions = append(minions, minionFromREST(minion))
	}

	paging.Token = resp.Payload.Paging.Token
	return minions, paging, nil
}

func (c *client) GetMinionMaster(ctx context.Context, fqdn string) (deployapi.MinionMaster, error) {
	req := swagminions.NewGetMinionMasterParamsWithContext(ctx)
	req = req.WithFqdn(fqdn)
	req = req.WithXRequestID(ptr.String(requestid.FromContextOrNew(ctx)))

	resp, err := c.cli.Minions.GetMinionMaster(req)
	if err != nil {
		return deployapi.MinionMaster{}, handleError(err)
	}

	return deployapi.MinionMaster{MasterFQDN: resp.Payload.Master, PublicKey: resp.Payload.PublicKey}, nil
}

func (c *client) RegisterMinion(ctx context.Context, fqdn string, pubKey string) (models.Minion, error) {
	req := swagminions.NewRegisterMinionParamsWithContext(ctx)
	req = req.WithAuthorization(c.getToken(req.Context))
	req = req.WithFqdn(fqdn)
	req = req.WithBody(&swagmodels.MinionPublicKey{PublicKey: pubKey})
	req = req.WithXRequestID(ptr.String(requestid.FromContextOrNew(ctx)))

	resp, err := c.cli.Minions.RegisterMinion(req)
	if err != nil {
		return models.Minion{}, handleError(err)
	}

	return minionFromREST(resp.Payload), nil
}

func (c *client) UnregisterMinion(ctx context.Context, fqdn string) (models.Minion, error) {
	req := swagminions.NewUnregisterMinionParamsWithContext(ctx)
	req = req.WithAuthorization(c.getToken(req.Context))
	req = req.WithFqdn(fqdn)
	req = req.WithXRequestID(ptr.String(requestid.FromContextOrNew(ctx)))

	resp, err := c.cli.Minions.UnregisterMinion(req)
	if err != nil {
		return models.Minion{}, handleError(err)
	}

	return minionFromREST(resp.Payload), nil
}

func (c *client) DeleteMinion(ctx context.Context, fqdn string) error {
	req := swagminions.NewDeleteMinionParamsWithContext(ctx)
	req = req.WithAuthorization(c.getToken(req.Context))
	req = req.WithFqdn(fqdn)
	req = req.WithXRequestID(ptr.String(requestid.FromContextOrNew(ctx)))

	_, err := c.cli.Minions.DeleteMinion(req)
	if err != nil {
		return handleError(err)
	}

	return nil
}

func (c *client) CreateShipment(
	ctx context.Context,
	fqdns []string,
	commands []models.CommandDef,
	parallel, stopOnErrorCount int64,
	timeout time.Duration,
) (models.Shipment, error) {
	req := swagcmds.NewCreateShipmentParamsWithContext(ctx)
	req = req.WithAuthorization(c.getToken(req.Context))
	req = req.WithBody(
		&swagmodels.Shipment{
			Fqdns:            fqdns,
			Commands:         commandsDefToREST(commands),
			Parallel:         parallel,
			StopOnErrorCount: stopOnErrorCount,
			Timeout:          int64(timeout.Seconds()),
		},
	)
	req = req.WithXRequestID(ptr.String(requestid.FromContextOrNew(ctx)))

	resp, err := c.cli.Commands.CreateShipment(req)
	if err != nil {
		return models.Shipment{}, handleError(err)
	}

	return shipmentFromREST(resp.Payload)
}

func (c *client) GetShipment(ctx context.Context, id models.ShipmentID) (models.Shipment, error) {
	req := swagcmds.NewGetShipmentParamsWithContext(ctx)
	req = req.WithAuthorization(c.getToken(req.Context))
	req = req.WithShipmentID(id.String())
	reqID := requestid.FromContextOrNew(ctx)
	req = req.WithXRequestID(&reqID)

	resp, err := c.cli.Commands.GetShipment(req)
	if err != nil {
		return models.Shipment{}, handleError(err)
	}

	return shipmentFromREST(resp.Payload)
}

func (c *client) GetShipments(ctx context.Context, attrs deployapi.SelectShipmentsAttrs, paging deployapi.Paging) ([]models.Shipment, deployapi.Paging, error) {
	req := swagcmds.NewGetShipmentsListParamsWithContext(ctx)
	req = req.WithAuthorization(c.getToken(req.Context))
	if attrs.FQDN.Valid {
		req = req.WithFqdn(&attrs.FQDN.String)
	}
	if attrs.Status.Valid {
		req = req.WithShipmentStatus(&attrs.Status.String)
	}

	if paging.HasSortOrder() {
		so, err := swagapi.SortOrderToREST(paging.SortOrder)
		if err != nil {
			return nil, deployapi.Paging{}, err
		}
		req = req.WithSortOrder(&so)
	}

	if paging.Size != 0 {
		req = req.WithPageSize(&paging.Size)
	}
	if paging.Token != "" {
		req = req.WithPageToken(&paging.Token)
	}
	req = req.WithXRequestID(ptr.String(requestid.FromContextOrNew(ctx)))

	resp, err := c.cli.Commands.GetShipmentsList(req)
	if err != nil {
		return nil, deployapi.Paging{}, handleError(err)
	}

	var shipments []models.Shipment
	for _, shipment := range resp.Payload.Shipments {
		s, err := shipmentFromREST(shipment)
		if err != nil {
			c.logger.Errorf("failed to convert shipment '%s' from REST: %s", shipment.ID, err)
			continue
		}

		shipments = append(shipments, s)
	}

	paging.Token = resp.Payload.Paging.Token
	return shipments, paging, nil
}

func (c *client) GetCommand(ctx context.Context, id models.CommandID) (models.Command, error) {
	req := swagcmds.NewGetCommandParamsWithContext(ctx)
	req = req.WithAuthorization(c.getToken(req.Context))
	req = req.WithCommandID(id.String())
	req = req.WithXRequestID(ptr.String(requestid.FromContextOrNew(ctx)))

	resp, err := c.cli.Commands.GetCommand(req)
	if err != nil {
		return models.Command{}, handleError(err)
	}

	return commandFromREST(resp.Payload)
}

func (c *client) GetCommands(ctx context.Context, attrs deployapi.SelectCommandsAttrs, paging deployapi.Paging) ([]models.Command, deployapi.Paging, error) {
	req := swagcmds.NewGetCommandsListParamsWithContext(ctx)
	req = req.WithAuthorization(c.getToken(req.Context))
	if attrs.ShipmentID.Valid {
		req = req.WithShipmentID(&attrs.ShipmentID.String)
	}
	if attrs.FQDN.Valid {
		req = req.WithFqdn(&attrs.FQDN.String)
	}
	if attrs.Status.Valid {
		req = req.WithCommandStatus(&attrs.Status.String)
	}

	if paging.HasSortOrder() {
		so, err := swagapi.SortOrderToREST(paging.SortOrder)
		if err != nil {
			return nil, deployapi.Paging{}, err
		}
		req = req.WithSortOrder(&so)
	}

	if paging.Size != 0 {
		req = req.WithPageSize(&paging.Size)
	}
	if paging.Token != "" {
		req = req.WithPageToken(&paging.Token)
	}
	req = req.WithXRequestID(ptr.String(requestid.FromContextOrNew(ctx)))

	resp, err := c.cli.Commands.GetCommandsList(req)
	if err != nil {
		return nil, deployapi.Paging{}, handleError(err)
	}

	var cmds []models.Command
	for _, cmd := range resp.Payload.Commands {
		cModel, err := commandFromREST(cmd)
		if err != nil {
			c.logger.Errorf("failed to convert command %+v from REST: %s", cmd, err)
			continue
		}

		cmds = append(cmds, cModel)
	}

	paging.Token = resp.Payload.Paging.Token
	return cmds, paging, nil
}

func (c *client) GetJob(ctx context.Context, id models.JobID) (models.Job, error) {
	req := swagcmds.NewGetJobParamsWithContext(ctx)
	req = req.WithAuthorization(c.getToken(req.Context))
	req = req.WithJobID(id.String())
	req = req.WithXRequestID(ptr.String(requestid.FromContextOrNew(ctx)))

	resp, err := c.cli.Commands.GetJob(req)
	if err != nil {
		return models.Job{}, handleError(err)
	}

	return jobFromREST(resp.Payload)
}

func (c *client) GetJobs(ctx context.Context, attrs deployapi.SelectJobsAttrs, paging deployapi.Paging) ([]models.Job, deployapi.Paging, error) {
	req := swagcmds.NewGetJobsListParamsWithContext(ctx)
	req = req.WithAuthorization(c.getToken(req.Context))
	if attrs.ShipmentID.Valid {
		req = req.WithShipmentID(&attrs.ShipmentID.String)
	}
	if attrs.ExtJobID.Valid {
		req = req.WithExtJobID(&attrs.ExtJobID.String)
	}
	if attrs.FQDN.Valid {
		req = req.WithFqdn(&attrs.FQDN.String)
	}
	if attrs.Status.Valid {
		req = req.WithJobStatus(&attrs.Status.String)
	}

	if paging.HasSortOrder() {
		so, err := swagapi.SortOrderToREST(paging.SortOrder)
		if err != nil {
			return nil, deployapi.Paging{}, err
		}
		req = req.WithSortOrder(&so)
	}

	if paging.Size != 0 {
		req = req.WithPageSize(&paging.Size)
	}
	if paging.Token != "" {
		req = req.WithPageToken(&paging.Token)
	}
	req = req.WithXRequestID(ptr.String(requestid.FromContextOrNew(ctx)))

	resp, err := c.cli.Commands.GetJobsList(req)
	if err != nil {
		return nil, deployapi.Paging{}, handleError(err)
	}

	var jobs []models.Job
	for _, job := range resp.Payload.Jobs {
		jModel, err := jobFromREST(job)
		if err != nil {
			c.logger.Errorf("failed to convert job %+v from REST: %s", job, err)
			continue
		}

		jobs = append(jobs, jModel)
	}

	paging.Token = resp.Payload.Paging.Token
	return jobs, paging, nil
}

func (c *client) CreateJobResult(ctx context.Context, id, fqdn string, result string) (models.JobResult, error) {
	req := swagcmds.NewCreateJobResultParamsWithContext(ctx)
	req = req.WithJobID(id)
	req = req.WithFqdn(fqdn)
	req = req.WithBody(
		&swagmodels.JobResult{
			Result: []byte(result),
		},
	)
	req = req.WithXRequestID(ptr.String(requestid.FromContextOrNew(ctx)))

	resp, err := c.cli.Commands.CreateJobResult(req)
	if err != nil {
		return models.JobResult{}, handleError(err)
	}

	return jobResultFromREST(resp.Payload), nil
}

func (c *client) GetJobResult(ctx context.Context, id models.JobResultID) (models.JobResult, error) {
	req := swagcmds.NewGetJobResultParamsWithContext(ctx)
	req = req.WithAuthorization(c.getToken(req.Context))
	req = req.WithJobResultID(id.String())
	req = req.WithXRequestID(ptr.String(requestid.FromContextOrNew(ctx)))

	resp, err := c.cli.Commands.GetJobResult(req)
	if err != nil {
		return models.JobResult{}, handleError(err)
	}

	return jobResultFromREST(resp.Payload), nil
}

func (c *client) GetJobResults(ctx context.Context, attrs deployapi.SelectJobResultsAttrs, paging deployapi.Paging) ([]models.JobResult, deployapi.Paging, error) {
	req := swagcmds.NewGetJobResultsListParamsWithContext(ctx)
	req = req.WithAuthorization(c.getToken(req.Context))
	if attrs.ExtJobID.Valid {
		req = req.WithJobID(&attrs.ExtJobID.String)
	}
	if attrs.FQDN.Valid {
		req = req.WithFqdn(&attrs.FQDN.String)
	}
	if attrs.Status.Valid {
		req = req.WithJobResultStatus(&attrs.Status.String)
	}

	if paging.HasSortOrder() {
		so, err := swagapi.SortOrderToREST(paging.SortOrder)
		if err != nil {
			return nil, deployapi.Paging{}, err
		}
		req = req.WithSortOrder(&so)
	}

	if paging.Size != 0 {
		req = req.WithPageSize(&paging.Size)
	}
	if paging.Token != "" {
		req = req.WithPageToken(&paging.Token)
	}
	req = req.WithXRequestID(ptr.String(requestid.FromContextOrNew(ctx)))

	resp, err := c.cli.Commands.GetJobResultsList(req)
	if err != nil {
		return nil, deployapi.Paging{}, handleError(err)
	}

	var jrs []models.JobResult
	for _, jr := range resp.Payload.JobResults {
		jrs = append(jrs, jobResultFromREST(jr))
	}

	paging.Token = resp.Payload.Paging.Token
	return jrs, paging, nil
}

func handleError(err error) error {
	e, ok := err.(swaggerError)
	if !ok {
		return err
	}

	err = xerrors.Errorf("[%d] %s", e.Code(), e.GetPayload().Message)

	switch {
	case e.Code() == 404:
		return deployapi.ErrNotFound.Wrap(err)
	case e.Code() >= 400 && e.Code() <= 499:
		return deployapi.ErrBadRequest.Wrap(err)
	case e.Code() == 503:
		return deployapi.ErrNotAvailable.Wrap(err)
	case e.Code() >= 500 && e.Code() <= 599:
		return deployapi.ErrInternalError.Wrap(err)
	}

	return err
}

type swaggerError interface {
	Code() int
	GetPayload() *swagmodels.Error
}
