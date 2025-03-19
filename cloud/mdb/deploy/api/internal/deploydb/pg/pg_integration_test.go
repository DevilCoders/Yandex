package pg_test

import (
	"context"
	"testing"
	"time"

	"github.com/gofrs/uuid"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/deploy/api/internal/deploydb"
	"a.yandex-team.ru/cloud/mdb/deploy/api/internal/deploydb/pg"
	"a.yandex-team.ru/cloud/mdb/deploy/api/pkg/models"
	"a.yandex-team.ru/cloud/mdb/deploydb/recipe/helpers"
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/internal/ready"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
)

func TestUnavailable(t *testing.T) {
	ctx, backend, _ := initPGAddrs(t, []string{"localhost:1", "localhost:2", "localhost:3"})
	defer func() { require.NoError(t, backend.Close()) }()

	// Test that deploydb is unavailable
	inserted, err := backend.CreateMaster(ctx, "", "", false, "")
	require.Error(t, err)
	require.True(t, semerr.IsUnavailable(err))
	require.Equal(t, models.Master{}, inserted)
}

func newGroup() models.Group {
	return models.Group{
		Name: uuid.Must(uuid.NewV4()).String(),
	}
}

func equalGroup(t *testing.T, expected, actual models.Group) {
	require.Equal(t, expected.Name, actual.Name)
}

func newMaster(groupName string) models.Master {
	return models.Master{
		FQDN:        uuid.Must(uuid.NewV4()).String(),
		Group:       groupName,
		IsOpen:      true,
		Description: uuid.Must(uuid.NewV4()).String(),
	}
}

func equalMaster(t *testing.T, expected, actual models.Master) {
	require.Equal(t, expected.Aliases, actual.Aliases)
	require.Equal(t, expected.FQDN, actual.FQDN)
	require.Equal(t, expected.Group, actual.Group)
	require.Equal(t, expected.Description, actual.Description)
	require.Equal(t, expected.IsOpen, actual.IsOpen)
	require.Equal(t, expected.Alive, actual.Alive)
}

func newMinion(groupName string) models.Minion {
	return models.Minion{
		FQDN:         uuid.Must(uuid.NewV4()).String(),
		Group:        groupName,
		AutoReassign: true,
	}
}

func equalMinion(t *testing.T, expected, actual models.Minion) {
	require.Equal(t, expected.FQDN, actual.FQDN)
	require.Equal(t, expected.Group, actual.Group)
	require.Equal(t, expected.MasterFQDN, actual.MasterFQDN)
	require.Equal(t, expected.AutoReassign, actual.AutoReassign)
}

func TestCreateGroup(t *testing.T) {
	ctx, backend, l := initPG(t)
	defer func() { require.NoError(t, backend.Close()) }()

	waitForBackend(ctx, t, backend, l)

	// Test group creation
	group := newGroup()
	inserted, err := backend.CreateGroup(ctx, group.Name)
	require.NoError(t, err)
	equalGroup(t, group, inserted)

	// Is group really created?
	loadedGroup, err := backend.Group(ctx, group.Name)
	require.NoError(t, err)
	equalGroup(t, group, loadedGroup)

	// Create another group
	group2 := newGroup()
	inserted2, err := backend.CreateGroup(ctx, group2.Name)
	require.NoError(t, err)
	equalGroup(t, group2, inserted2)

	// Is first group still accessible?
	loadedGroup, err = backend.Group(ctx, group.Name)
	require.NoError(t, err)
	equalGroup(t, group, loadedGroup)

	// Is another group really created?
	loadedGroup2, err := backend.Group(ctx, group2.Name)
	require.NoError(t, err)
	equalGroup(t, group2, loadedGroup2)
}

func TestCreateMaster(t *testing.T) {
	ctx, backend, l := initPG(t)
	defer func() { require.NoError(t, backend.Close()) }()

	waitForBackend(ctx, t, backend, l)

	// Create group
	group := newGroup()
	insertedGroup, err := backend.CreateGroup(ctx, group.Name)
	require.NoError(t, err)
	equalGroup(t, group, insertedGroup)

	// Create master in group
	master := newMaster(group.Name)
	insertedMaster, err := backend.CreateMaster(ctx, master.FQDN, master.Group, master.IsOpen, master.Description)
	require.NoError(t, err)
	equalMaster(t, master, insertedMaster)

	// Is master really created?
	loadedMaster, err := backend.Master(ctx, master.FQDN)
	require.NoError(t, err)
	equalMaster(t, master, loadedMaster)
}

func TestCreateMinion(t *testing.T) {
	ctx, backend, l := initPG(t)
	defer func() { require.NoError(t, backend.Close()) }()

	waitForBackend(ctx, t, backend, l)

	// Create group
	group := newGroup()
	insertedGroup, err := backend.CreateGroup(ctx, group.Name)
	require.NoError(t, err)
	equalGroup(t, group, insertedGroup)

	// Create master in group
	master := newMaster(group.Name)
	insertedMaster, err := backend.CreateMaster(ctx, master.FQDN, master.Group, master.IsOpen, master.Description)
	require.NoError(t, err)
	equalMaster(t, master, insertedMaster)

	// Is master really created?
	loadedMaster, err := backend.Master(ctx, master.FQDN)
	require.NoError(t, err)
	equalMaster(t, master, loadedMaster)

	// Mark master alive
	aliveMaster, err := backend.UpdateMasterCheck(ctx, master.FQDN, uuid.Must(uuid.NewV4()).String(), true)
	require.NoError(t, err)
	master.Alive = true
	equalMaster(t, master, aliveMaster)

	// Create minion in group
	minion := newMinion(group.Name)
	insertedMinion, err := backend.CreateMinion(ctx, minion.FQDN, minion.Group, minion.AutoReassign)
	require.NoError(t, err)
	minion.MasterFQDN = master.FQDN
	equalMinion(t, minion, insertedMinion)

	// Is minion really created?
	loadedMinion, err := backend.Minion(ctx, minion.FQDN)
	require.NoError(t, err)
	equalMinion(t, minion, loadedMinion)
}

func TestCleanupUnboundJobResults(t *testing.T) {
	ctx, backend, l := initPG(t)
	defer func() { require.NoError(t, backend.Close()) }()

	waitForBackend(ctx, t, backend, l)

	count, err := backend.CleanupUnboundJobResults(ctx, 0, 100)
	require.NoError(t, err)
	require.Equal(t, uint64(0), count)

	_, err = backend.CreateJobResult(ctx, "1", "minion2", models.JobResultStatusSuccess, []byte("{}"))
	require.NoError(t, err)

	count, err = backend.CleanupUnboundJobResults(ctx, 0, 100)
	require.NoError(t, err)
	require.Equal(t, uint64(1), count)
}

func waitForBackend(ctx context.Context, t *testing.T, backend deploydb.Backend, l log.Logger) {
	require.NoError(t, ready.Wait(ctx, backend, &ready.DefaultErrorTester{Name: "db_pg_test", L: l}, time.Second))
}

func initPG(t *testing.T) (context.Context, deploydb.Backend, log.Logger) {
	return initPGAddrs(t, nil)
}

func initPGAddrs(t *testing.T, addrs []string) (context.Context, deploydb.Backend, log.Logger) {
	ctx := context.Background()
	logger, _ := zap.New(zap.KVConfig(log.DebugLevel))

	cfg := helpers.PGConfig()

	if addrs != nil {
		cfg.Addrs = addrs
	}

	backend, err := pg.New(cfg, logger)
	require.NoError(t, err)
	require.NotNil(t, backend)
	return ctx, backend, logger
}

func TestListJobResults(t *testing.T) {
	ctx, backend, l := initPG(t)
	defer func() { require.NoError(t, backend.Close()) }()

	waitForBackend(ctx, t, backend, l)

	results, err := backend.JobResults(ctx, deploydb.SelectJobResultsAttrs{}, 100, optional.Int64{})
	require.NoError(t, err)
	require.Equal(t, 0, len(results))

	_, err = backend.CreateJobResult(ctx, "1", "minion2", models.JobResultStatusSuccess, []byte("{}"))
	require.NoError(t, err)

	// Search another minion's results
	results, err = backend.JobResults(ctx, deploydb.SelectJobResultsAttrs{FQDN: optional.NewString("minion1")}, 100, optional.Int64{})
	require.NoError(t, err)
	require.Equal(t, 0, len(results))

	results, err = backend.JobResults(ctx, deploydb.SelectJobResultsAttrs{FQDN: optional.NewString("minion2")}, 100, optional.Int64{})
	require.NoError(t, err)
	require.Equal(t, 1, len(results))
}

func TestListJobResultsWithPaging(t *testing.T) {
	ctx, backend, l := initPG(t)
	defer func() { require.NoError(t, backend.Close()) }()

	waitForBackend(ctx, t, backend, l)

	results, err := backend.JobResults(ctx, deploydb.SelectJobResultsAttrs{FQDN: optional.NewString("minion3")}, 100, optional.Int64{})
	require.NoError(t, err)
	require.Equal(t, 0, len(results))

	first, err := backend.CreateJobResult(ctx, "2", "minion3", models.JobResultStatusSuccess, []byte("{}"))
	require.NoError(t, err)
	second, err := backend.CreateJobResult(ctx, "2", "minion3", models.JobResultStatusSuccess, []byte("{}"))
	require.NoError(t, err)

	results, err = backend.JobResults(ctx, deploydb.SelectJobResultsAttrs{FQDN: optional.NewString("minion3")}, 100, optional.Int64{})
	require.NoError(t, err)
	require.Equal(t, 2, len(results))

	results, err = backend.JobResults(ctx, deploydb.SelectJobResultsAttrs{FQDN: optional.NewString("minion3")}, 100, optional.NewInt64(0))
	require.NoError(t, err)
	require.Equal(t, 2, len(results))

	results, err = backend.JobResults(ctx, deploydb.SelectJobResultsAttrs{FQDN: optional.NewString("minion3")}, 100, optional.NewInt64(int64(first.JobResultID)))
	require.NoError(t, err)
	require.Equal(t, 1, len(results))
	require.Equal(t, second.ExtID, results[0].ExtID)

	results, err = backend.JobResults(ctx, deploydb.SelectJobResultsAttrs{FQDN: optional.NewString("minion3"), SortOrder: models.SortOrderDesc}, 100, optional.NewInt64(int64(second.JobResultID)))
	require.NoError(t, err)
	require.Equal(t, 1, len(results))
	require.Equal(t, first.ExtID, results[0].ExtID)
}
