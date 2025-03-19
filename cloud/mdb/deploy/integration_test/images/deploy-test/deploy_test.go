package deploy_test

import (
	"context"
	"fmt"
	"os"
	"strings"
	"testing"
	"time"

	"github.com/gofrs/uuid"
	"github.com/stretchr/testify/require"
	"golang.org/x/sync/errgroup"

	"a.yandex-team.ru/cloud/mdb/deploy/api/pkg/deployapi"
	"a.yandex-team.ru/cloud/mdb/deploy/api/pkg/deployapi/restapi"
	"a.yandex-team.ru/cloud/mdb/deploy/api/pkg/models"
	"a.yandex-team.ru/cloud/mdb/internal/encodingutil"
	"a.yandex-team.ru/cloud/mdb/internal/httputil"
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/ready"
	"a.yandex-team.ru/cloud/mdb/internal/saltapi"
	"a.yandex-team.ru/cloud/mdb/internal/saltapi/cherrypy"
	"a.yandex-team.ru/cloud/mdb/internal/secret"
	"a.yandex-team.ru/cloud/mdb/internal/supervisor"
	"a.yandex-team.ru/cloud/mdb/internal/supervisor/remote"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func init() {
	state.masters = map[string]models.Master{}
	state.minions = map[string]models.Minion{}
}

type State struct {
	group   models.Group
	masters map[string]models.Master
	minions map[string]models.Minion
}

var state State

func (s State) mastersFQDN() []string {
	fqdns := make([]string, 0, len(s.masters))
	for fqdn := range s.masters {
		fqdns = append(fqdns, fqdn)
	}

	return fqdns
}

func (s State) minionsFQDN() []string {
	fqdns := make([]string, 0, len(s.minions))
	for fqdn := range s.minions {
		fqdns = append(fqdns, fqdn)
	}

	return fqdns
}

func (s State) firstMinion() models.Minion {
	for _, m := range s.minions {
		return m
	}

	panic("no minions found")
}

const (
	groupName = "deploy-test"
)

// TestGroup for entire deploy test (create group, get it, etc)
func TestGroup(t *testing.T) {
	ctx, dapi, lg := initTest(t)
	// Time limit
	ctx, cancel := context.WithTimeout(ctx, time.Minute*3)
	defer cancel()

	waitForDeployAPI(ctx, t, dapi, lg)

	// Check that group is not there
	groupLoaded, err := dapi.GetGroup(ctx, groupName)
	require.Error(t, err)
	require.True(t, xerrors.Is(err, deployapi.ErrNotFound))
	require.Equal(t, models.Group{}, groupLoaded)

	// Create group
	state.group, err = dapi.CreateGroup(ctx, groupName)
	require.NoError(t, err)
	require.NotEqual(t, models.Group{}, state.group)

	// Get group
	groupLoaded, err = dapi.GetGroup(ctx, groupName)
	require.NoError(t, err)
	require.Equal(t, state.group, groupLoaded)

	// Get group list
	groups, paging, err := dapi.GetGroups(ctx, deployapi.Paging{})
	require.NoError(t, err)
	require.Len(t, groups, 1)
	require.Equal(t, state.group, groups[0])
	require.Equal(t, deployapi.Paging{Token: groups[0].ID.String()}, paging)
}

// TestSetupMastersAndMinions performs masters and minions setup (registers them, waits for key accepts, etc)
func TestSetupMastersAndMinions(t *testing.T) {
	ctx, dapi, lg := initTest(t)
	// Time limit
	ctx, cancel := context.WithTimeout(ctx, time.Minute*5)
	defer cancel()

	waitForDeployAPI(ctx, t, dapi, lg)

	// Create all masters and their minions
	// TODO: parallel
	for _, fqdn := range saltSaltMastersFQDN() {
		created, err := dapi.CreateMaster(
			ctx,
			fqdn,
			state.group.Name,
			true,
			"",
		)
		require.NoError(t, err)
		require.NotEqual(t, models.Master{}, created)

		state.masters[created.FQDN] = created

		// Wait for master to become alive, otherwise we will fail to create minions
		require.NoError(t, waitForAliveMaster(ctx, dapi, fqdn, lg))
	}

	// Create all minions
	for _, fqdn := range saltMinionsFQDN() {
		setupMinion(ctx, t, dapi, fqdn)
	}

	// Wait for all minions to become alive (do 'test.ping' with salt)
	g, ctx := errgroup.WithContext(ctx)
	for _, master := range state.masters {
		salt, auth := newSaltClient(ctx, t, master, lg)

		for _, minion := range state.minions {
			if minion.MasterFQDN != master.FQDN {
				continue
			}

			fqdn := minion.FQDN
			g.Go(func() error { return initialMinionCheck(ctx, dapi, salt, auth, fqdn, lg) })
		}
	}
	require.NoError(t, g.Wait())
}

func newSaltClient(ctx context.Context, t *testing.T, master models.Master, lg log.Logger) (saltapi.Client, saltapi.Secrets) {
	// Create salt-api client
	salt, err := cherrypy.New(
		"https://"+master.FQDN,
		httputil.TLSConfig{CAFile: caPath()},
		httputil.LoggingConfig{
			LogRequestBody:  true,
			LogResponseBody: true,
		},
		lg,
	)
	require.NoError(t, err)
	require.NotNil(t, salt)

	// Create credentials
	auth := salt.NewAuth(saltAPICredentials())
	require.NotNil(t, auth)

	// Authenticate
	_, err = auth.AuthSessionToken(ctx)
	require.NoError(t, err)
	_, err = auth.AuthEAuthToken(ctx, 15*time.Second)
	require.NoError(t, err)

	return salt, auth
}

// initialMinionCheck pings minion, then does saltutil.sync_all and state.highstate
func initialMinionCheck(ctx context.Context, dapi deployapi.Client, salt saltapi.Client, auth saltapi.Secrets, fqdn string, lg log.Logger) error {
	ticker := time.NewTicker(time.Second)
	defer ticker.Stop()

	for {
		select {
		case <-ctx.Done():
			return ctx.Err()
		case <-ticker.C:
			// Ping minion synchronously for speed
			res, err := salt.Test().Ping(ctx, auth, 15*time.Second, fqdn)
			if err != nil {
				lg.Infof("Error while doing ping for minion '%s': %s", fqdn, err)
				continue
			}

			lg.Infof("Ping result for '%s': %+v", fqdn, res)

			// Did ping succeed?
			ping, ok := res[fqdn]
			if ok && ping {
				// Run saltutil.sync_all on minion
				syncAllCmd := models.CommandDef{Type: "saltutil.sync_all", Args: nil, Timeout: encodingutil.FromDuration(time.Minute * 5)}
				lg.Infof("Sending sync_all to '%s'", fqdn)
				shipment, err := dapi.CreateShipment(ctx, []string{fqdn}, []models.CommandDef{syncAllCmd}, 1, 0, time.Minute*5)
				if err != nil {
					return fmt.Errorf("error while doing sync_all for minion '%s': %s", fqdn, err)
				}

				lg.Infof("Waiting for sync_all on '%s'", fqdn)
				shipment, err = waitForShipment(ctx, dapi, shipment.ID, []models.ShipmentStatus{models.ShipmentStatusDone, models.ShipmentStatusError}, lg)
				if err != nil {
					return err
				}

				if shipment.Status != models.ShipmentStatusDone {
					return fmt.Errorf("shipment '%d' status is '%s' instead of '%s'", shipment.ID, shipment.Status, models.ShipmentStatusDone)
				}

				lg.Infof("Succeeded sync_all on '%s'", fqdn)

				// Run state.highstate on minion
				highstateCmd := models.CommandDef{Type: "state.highstate", Args: []string{"queue=True"}, Timeout: encodingutil.FromDuration(time.Minute * 5)}
				lg.Infof("Sending highstate to '%s'", fqdn)
				shipment, err = dapi.CreateShipment(ctx, []string{fqdn}, []models.CommandDef{highstateCmd}, 1, 0, time.Minute*5)
				if err != nil {
					return fmt.Errorf("error while doing highstate for minion '%s': %s", fqdn, err)
				}

				lg.Infof("Waiting for highstate on '%s'", fqdn)
				shipment, err = waitForShipment(ctx, dapi, shipment.ID, []models.ShipmentStatus{models.ShipmentStatusDone, models.ShipmentStatusError}, lg)
				if err != nil {
					return err
				}

				if shipment.Status != models.ShipmentStatusDone {
					return fmt.Errorf("shipment '%d' status is '%s' instead of '%s'", shipment.ID, shipment.Status, models.ShipmentStatusDone)
				}

				lg.Infof("Succeeded highstate on '%s'", fqdn)

				// Verify that all successful job results exist
				jrs, _, err := dapi.GetJobResults(
					ctx,
					deployapi.SelectJobResultsAttrs{
						FQDN:   optional.NewString(fqdn),
						Status: optional.NewString("success"),
					},
					deployapi.Paging{Size: 1000},
				)
				if err != nil {
					return err
				}

				// TODO: validate that specifically ping, sync_all and hs exist (
				const expectedJRCount = 3
				if len(jrs) < expectedJRCount {
					return xerrors.Errorf("expected at least %d successful job results, but got only %d: %+v", expectedJRCount, len(jrs), jrs)
				}

				lg.Infof("Number of successful job results for '%s' is '%d' which is at least what we expected (%d)", fqdn, len(jrs), expectedJRCount)
				return nil
			}
		}
	}
}

func setupMinion(ctx context.Context, t *testing.T, dapi deployapi.Client, fqdn string) {
	const (
		ar = true
	)

	minion, err := dapi.CreateMinion(ctx, fqdn, state.group.Name, ar)
	require.NoError(t, err)
	require.Equal(t, fqdn, minion.FQDN)
	require.Contains(t, state.mastersFQDN(), minion.MasterFQDN)
	require.Equal(t, ar, minion.AutoReassign)
	state.minions[minion.FQDN] = minion
}

// TestMinions tests that current status of minions according to salt master is exactly what we expect
func TestMinions(t *testing.T) {
	ctx, dapi, lg := initTest(t)
	// Time limit
	ctx, cancel := context.WithTimeout(ctx, time.Minute*1)
	defer cancel()

	waitForDeployAPI(ctx, t, dapi, lg)

	var minions []saltapi.Minion
	for _, master := range state.masters {
		salt, auth := newSaltClient(ctx, t, master, lg)

		loaded, err := salt.Minions(ctx, auth)
		require.NoError(t, err)
		minions = append(minions, loaded...)
	}

	lg.Infof("minions: %+v", minions)

	checkMinions(t, state.minionsFQDN(), minions, true)
}

func checkMinions(t *testing.T, expected []string, minions []saltapi.Minion, accepted bool) {
	require.Len(t, minions, len(expected))

	for _, m := range minions {
		require.Equal(t, accepted, m.Accepted)
	}
}

func TestJobTimeout(t *testing.T) {
	ctx, dapi, lg := initTest(t)
	// Time limit
	ctx, cancel := context.WithTimeout(ctx, time.Minute*2)
	defer cancel()

	m := state.firstMinion()
	const cmdType = "test.sleep"

	lg.Infof("Sending %s to %q for job timeout", cmdType, m.FQDN)
	cmd := models.CommandDef{
		Type:    cmdType,
		Args:    []string{"600"},
		Timeout: encodingutil.FromDuration(time.Second),
	}
	shipment, err := dapi.CreateShipment(ctx, []string{m.FQDN}, []models.CommandDef{cmd}, 1, 1, time.Minute*10)
	require.NoError(t, err)

	lg.Infof("Waiting job timeout on %q", m.FQDN)
	shipment, err = waitForShipment(
		ctx, dapi, shipment.ID,
		[]models.ShipmentStatus{models.ShipmentStatusDone, models.ShipmentStatusError, models.ShipmentStatusTimeout},
		lg,
	)
	require.NoError(t, err)
	require.Equal(t, models.ShipmentStatusError, shipment.Status)

	var attrs deployapi.SelectCommandsAttrs
	attrs.ShipmentID.Set(shipment.ID.String())
	cmds, _, err := dapi.GetCommands(ctx, attrs, deployapi.Paging{})
	require.NoError(t, err)
	lg.Infof("commands: %+v", cmds)
	require.Len(t, cmds, 1)
	require.Equal(t, models.CommandStatusTimeout, cmds[0].Status)

	lg.Infof("Job timeout successful on %q", m.FQDN)
}

func TestRunningJob(t *testing.T) {
	ctx, dapi, lg := initTest(t)
	// Time limit
	ctx, cancel := context.WithTimeout(ctx, time.Minute)
	defer cancel()

	m := state.firstMinion()
	const cmdType = "test.sleep"
	lg.Infof("Sending %s to %q", cmdType, m.FQDN)
	cmd := models.CommandDef{
		Type:    cmdType,
		Args:    []string{"3600"},
		Timeout: encodingutil.FromDuration(time.Hour),
	}
	shipment, err := dapi.CreateShipment(ctx, []string{m.FQDN}, []models.CommandDef{cmd}, 1, 1, time.Hour)
	require.NoError(t, err)

	// Send kill order
	require.NoError(t, killFuncOnMinion(ctx, t, cmdType, m, lg))

	lg.Infof("Waiting shipment failure on %q", m.FQDN)
	shipment, err = waitForShipment(
		ctx, dapi, shipment.ID,
		[]models.ShipmentStatus{models.ShipmentStatusDone, models.ShipmentStatusError, models.ShipmentStatusTimeout},
		lg,
	)
	require.NoError(t, err)
	require.Equal(t, models.ShipmentStatusError, shipment.Status)

	// Get job for that shipment
	var jobsattrs deployapi.SelectJobsAttrs
	jobsattrs.ShipmentID.Set(shipment.ID.String())
	jobs, _, err := dapi.GetJobs(ctx, jobsattrs, deployapi.Paging{})
	require.NoError(t, err)
	require.Len(t, jobs, 1)

	// Checking that job result was 'NOTRUNNING'
	var attrs deployapi.SelectJobResultsAttrs
	attrs.ExtJobID.Set(jobs[0].ExtID)
	jrs, _, err := dapi.GetJobResults(ctx, attrs, deployapi.Paging{})
	require.NoError(t, err)
	require.Len(t, jrs, 1)
	require.Equal(t, models.JobResultStatusNotRunning, jrs[0].Status)

	lg.Infof("Job successfully killed and detected as not running %q", m.FQDN)
}

func killFuncOnMinion(ctx context.Context, t *testing.T, f string, m models.Minion, lg log.Logger) error {
	lg.Infof("Killing %q on minion %q", f, m.FQDN)
	salt, auth := newSaltClient(ctx, t, state.masters[m.MasterFQDN], lg)
	for {
		select {
		case <-ctx.Done():
			return ctx.Err()
		case <-time.After(time.Second):
			res, err := salt.SaltUtil().IsRunning(ctx, auth, time.Minute, m.FQDN, f)
			require.NoError(t, err)

			running, ok := res[m.FQDN]
			if !ok {
				lg.Infof("Func %q not found on minion %q", f, m.FQDN)
				continue
			}

			lg.Debugf("Received running for minion %q: %+v", m.FQDN, running)

			var found bool
			for _, r := range running {
				if r.Function != f {
					continue
				}

				lg.Infof("Sending kill order for %q on minion %q", f, m.FQDN)
				require.NoError(t, salt.SaltUtil().KillJob(ctx, auth, time.Minute, m.FQDN, r.JobID))
				lg.Infof("Killed %q on minion %q", f, m.FQDN)
				found = true
			}

			if found {
				return nil
			}
		}
	}
}

func TestShipmentTimeout(t *testing.T) {
	ctx, dapi, lg := initTest(t)
	// Time limit
	ctx, cancel := context.WithTimeout(ctx, time.Minute*2)
	defer cancel()

	m := state.firstMinion()
	srvs := initSupervisorClient(t, m.FQDN, lg)

	// Stop salt-minion so that it won't be able to handle commands
	require.NoError(t, srvs.Stop("salt-minion"))

	lg.Infof("Sending saltutil.sync_all and state.highstate to '%s' for shipment timeout", m.FQDN)
	cmdDefs := []models.CommandDef{
		{
			Type:    "saltutil.sync_all",
			Timeout: encodingutil.FromDuration(time.Minute * 10),
		},
		{
			Type:    "state.highstate",
			Args:    []string{"queue=True"},
			Timeout: encodingutil.FromDuration(time.Minute * 10),
		},
	}
	shipment, err := dapi.CreateShipment(ctx, []string{m.FQDN}, cmdDefs, 1, 1, time.Second)
	require.NoError(t, err)

	lg.Infof("Waiting shipment timeout on '%s'", m.FQDN)
	shipment, err = waitForShipment(
		ctx, dapi, shipment.ID,
		[]models.ShipmentStatus{models.ShipmentStatusDone, models.ShipmentStatusError, models.ShipmentStatusTimeout},
		lg,
	)
	require.NoError(t, err)
	require.Equal(t, models.ShipmentStatusTimeout, shipment.Status)

	var attrs deployapi.SelectCommandsAttrs
	attrs.ShipmentID.Set(shipment.ID.String())
	cmds, _, err := dapi.GetCommands(ctx, attrs, deployapi.Paging{})
	require.NoError(t, err)
	lg.Infof("commands: %+v", cmds)
	require.Len(t, cmds, 2)
	// Only second command will be canceled (which was blocked before timeout)
	require.Equal(t, models.CommandStatusCanceled, cmds[1].Status)

	lg.Infof("Shipment timeout successful on '%s'", m.FQDN)

	// Restart salt-minion
	require.NoError(t, srvs.Start("salt-minion"))
}

func TestFailover(t *testing.T) {
	ctx, dapi, lg := initTest(t)
	// Time limit
	ctx, cancel := context.WithTimeout(ctx, time.Minute*7)
	defer cancel()

	masters, _, err := dapi.GetMasters(ctx, deployapi.Paging{})
	require.NoError(t, err)
	require.NotNil(t, masters)
	lg.Infof("mastesr: %+v", masters)
	require.Len(t, masters, len(state.masters))

	// Fail each master, check failover and then restore
	for i, master := range masters {
		// TODO: yeah... but it works
		targetMaster := masters[(i-1)*-1]
		// Save current state of minions
		minions, _, err := dapi.GetMinions(ctx, deployapi.Paging{})
		require.NoError(t, err)
		require.NotNil(t, minions)

		var found bool
		for _, minion := range minions {
			if minion.MasterFQDN == master.FQDN {
				found = true
				break
			}
		}
		require.True(t, found)

		srvs := initSupervisorClient(t, master.FQDN, lg)

		// Stop salt-master
		require.NoError(t, srvs.Stop("salt-api"))
		require.NoError(t, srvs.Stop("salt-master"))

		require.NoError(t, waitForReassign(ctx, dapi, master, lg))
		salt, auth := newSaltClient(ctx, t, targetMaster, lg)
		require.NoError(t, waitForFailover(ctx, salt, auth, minions, targetMaster, lg))

		// Restart salt-master
		require.NoError(t, srvs.Start("salt-api"))
		require.NoError(t, srvs.Start("salt-master"))

		// TODO: There is no rebalacing yet, test it when its in
	}
}

// TestMinionsCreateAndDelete tests that we can create and delete minions
// TODO: move this higher after minions listing can filter out deleted minions (so that code of failover
// test won't be ugly)
func TestMinionCreateAndDelete(t *testing.T) {
	ctx, dapi, lg := initTest(t)
	// Time limit
	ctx, cancel := context.WithTimeout(ctx, time.Second*30)
	defer cancel()

	waitForDeployAPI(ctx, t, dapi, lg)

	fqdnUUID, err := uuid.NewV4()
	require.NoError(t, err)
	fqdn := fqdnUUID.String()

	// Check that minion is not there
	minionLoaded, err := dapi.GetMinion(ctx, fqdn)
	require.Error(t, err)
	require.True(t, xerrors.Is(err, deployapi.ErrNotFound))
	require.Equal(t, models.Minion{}, minionLoaded)

	// Delete nonexistent minion
	err = dapi.DeleteMinion(ctx, fqdn)
	require.Error(t, err)
	require.True(t, xerrors.Is(err, deployapi.ErrNotFound))

	// Create minion
	minionCreated, err := dapi.CreateMinion(ctx, fqdn, state.group.Name, true)
	require.NoError(t, err)
	require.Equal(t, fqdn, minionCreated.FQDN)

	// Get minion
	minionLoaded, err = dapi.GetMinion(ctx, fqdn)
	require.NoError(t, err)
	require.Equal(t, minionCreated, minionLoaded)

	// Delete minion
	require.NoError(t, dapi.DeleteMinion(ctx, fqdn))

	// Check that minion is deleted
	minionLoaded, err = dapi.GetMinion(ctx, fqdn)
	require.NoError(t, err)
	deletedMinion := minionCreated
	deletedMinion.Deleted = true
	deletedMinion.UpdatedAt = minionLoaded.UpdatedAt //time may pass before deletion
	require.Equal(t, deletedMinion, minionLoaded)
}

func initTest(t *testing.T) (context.Context, deployapi.Client, log.Logger) {
	ctx := context.Background()

	l, err := zap.New(zap.KVConfig(log.DebugLevel))
	require.NoError(t, err)
	require.NotNil(t, l)

	dapi, err := restapi.New(
		deployAPIURL(),
		deployAPIToken(),
		nil,
		httputil.TLSConfig{CAFile: caPath()},
		httputil.LoggingConfig{},
		l,
	)
	require.NoError(t, err)
	require.NotNil(t, dapi)

	return ctx, dapi, l
}

func initSupervisorClient(t *testing.T, fqdn string, lg log.Logger) supervisor.Services {
	srvs := remote.New(
		fmt.Sprintf("http://%s:%s", fqdn, supervisorPort()),
		supervisorUser(),
		supervisorPassword(),
		lg,
	)
	require.NotNil(t, srvs)
	return srvs
}

const (
	envNameDeployAPIURL       = "MDB_DEPLOY_API_URL"
	envNameDeployAPIToken     = "MDB_DEPLOY_API_TOKEN"
	envNameCAPath             = "MDB_CA_PATH"
	envNameSaltMastersFQDN    = "MDB_SALT_MASTERS_FQDN"
	envNameSaltMinionsFQDN    = "MDB_SALT_MINIONS_FQDN"
	envNameSaltAPIUser        = "MDB_SALT_API_USER"
	envNameSaltAPIPassword    = "MDB_SALT_API_PASSWORD"
	envNameSupervisorPort     = "MDB_SUPERVISOR_PORT"
	envNameSupervisorUser     = "MDB_SUPERVISOR_USER"
	envNameSupervisorPassword = "MDB_SUPERVISOR_PASSWORD"
)

func envVar(name, def string) string {
	if v, ok := os.LookupEnv(name); ok {
		fmt.Printf("Found env var %q: %s\n", name, v)
		return v
	}

	fmt.Printf("Using default value for env var %q: %s\n", name, def)
	return def
}

func deployAPIURL() string {
	return envVar(envNameDeployAPIURL, "https://mdb-deploy-api")
}

func deployAPIToken() string {
	return envVar(envNameDeployAPIToken, "testtoken")
}

func caPath() string {
	return envVar(envNameCAPath, "/opt/yandex/allCAs.pem")
}

func saltSaltMastersFQDN() []string {
	return strings.Split(envVar(envNameSaltMastersFQDN, "1.salt-master;2.salt-master"), ";")
}

func saltMinionsFQDN() []string {
	return strings.Split(envVar(envNameSaltMinionsFQDN, "1.salt-minion;2.salt-minion"), ";")
}

func saltAPICredentials() saltapi.Credentials {
	return saltapi.Credentials{User: saltAPIUser(), Password: secret.NewString(saltAPIPassword()), EAuth: saltAPIEAuth()}
}

func saltAPIUser() string {
	return envVar(envNameSaltAPIUser, "saltapi")
}

func saltAPIPassword() string {
	return envVar(envNameSaltAPIPassword, "testpwd")
}
func saltAPIEAuth() string {
	return saltapi.EAuthPAM
}

func supervisorUser() string {
	return envVar(envNameSupervisorUser, "supervisor")
}

func supervisorPassword() string {
	return envVar(envNameSupervisorPassword, "testpwd")
}

func supervisorPort() string {
	return envVar(envNameSupervisorPort, "33661")
}

func waitForAliveMaster(ctx context.Context, dapi deployapi.Client, fqdn string, l log.Logger) error {
	ticker := time.NewTicker(time.Second)
	defer ticker.Stop()

	for {
		select {
		case <-ctx.Done():
			return ctx.Err()
		case <-ticker.C:
			master, err := dapi.GetMaster(ctx, fqdn)
			if err != nil {
				l.Errorf("Error waiting for master to become alive: %s", err)
				continue
			}

			if !master.Alive {
				l.Infof("Master '%s' is dead", fqdn)
				continue
			}

			l.Infof("Master '%s' is alive", fqdn)
			return nil
		}
	}
}

func waitForDeployAPI(ctx context.Context, t *testing.T, dapi deployapi.Client, l log.Logger) {
	require.NoError(t, ready.Wait(ctx, dapi, &ready.DefaultErrorTester{Name: "deploy test", L: l}, time.Second))
	l.Info("Deploy API is ready")
}

func waitForReassign(ctx context.Context, dapi deployapi.Client, master models.Master, lg log.Logger) error {
	ticker := time.NewTicker(time.Second)
	defer ticker.Stop()

	for {
		select {
		case <-ctx.Done():
			return ctx.Err()
		case <-ticker.C:
			minions, _, err := dapi.GetMinions(ctx, deployapi.Paging{})
			if err != nil {
				return err
			}

			reassigned := true
			for _, minion := range minions {
				if minion.MasterFQDN == master.FQDN {
					lg.Infof("Minion '%s' is still on failed master", minion.FQDN)
					reassigned = false
				}
			}

			if !reassigned {
				continue
			}

			lg.Infof("Reassign succeeded for master '%s'", master.FQDN)
			return nil
		}
	}
}

func waitForShipment(ctx context.Context, dapi deployapi.Client, id models.ShipmentID, statuses []models.ShipmentStatus, lg log.Logger) (models.Shipment, error) {
	ticker := time.NewTicker(time.Second)
	defer ticker.Stop()

	for {
		select {
		case <-ctx.Done():
			lg.Errorf("Failed to wait for shipment '%d' with one of the following statuses '%+v'", id, statuses)
			return models.Shipment{}, ctx.Err()
		case <-ticker.C:
			shipment, err := dapi.GetShipment(ctx, id)
			if err != nil {
				lg.Errorf("Error while retrieving shipment '%d': %s", id, err)
				continue
			}

			for _, s := range statuses {
				if shipment.Status == s {
					return shipment, nil
				}
			}
		}
	}
}

func waitForFailover(ctx context.Context, salt saltapi.Client, auth saltapi.Secrets, minions []models.Minion, master models.Master, lg log.Logger) error {
	ticker := time.NewTicker(time.Second)
	defer ticker.Stop()

	for {
		select {
		case <-ctx.Done():
			return ctx.Err()
		case <-ticker.C:
			// Get current minions on master
			assigned, err := salt.Minions(ctx, auth)
			if err != nil {
				return err
			}

			// Check number of minions
			if len(assigned) != len(minions) {
				lg.Warnf(
					"Number of minions on salt master (%d) is not whats expected (%d)",
					len(assigned),
					len(minions),
				)
				continue
			}

			lg.Infof("Number of minions on salt master is whats expected (%d)", len(minions))

			// Ping all minions attached to master
			res, err := salt.Test().Ping(ctx, auth, time.Minute, "*")
			if err != nil {
				return err
			}

			// Check number of results
			if len(res) != len(minions) {
				lg.Warnf(
					"Number of pinged minions on salt master (%d) is not whats expected (%d)",
					len(res),
					len(minions),
				)
				continue
			}

			lg.Infof("Number of pinged minions on salt master is whats expected (%d)", len(minions))

			// Check ping results
			failover := true
			for _, minion := range minions {
				ping, ok := res[minion.FQDN]
				if !ok {
					// This minion wasn't in the result set which means its not on salt master
					return xerrors.Errorf(
						"Didn't ping minion '%s' on master '%s' when was expected to",
						minion.FQDN,
						master.FQDN,
					)
				}

				// Did ping succeed?
				if !ping {
					lg.Warnf("Failed to ping minion '%s'", minion.FQDN)
					failover = false
					break
				}
			}

			if !failover {
				continue
			}

			lg.Infof("Failover succeeded for master '%s'", master.FQDN)
			return nil
		}
	}
}
