package testhelpers

import (
	"context"
	"reflect"
	"testing"

	"github.com/golang/mock/gomock"

	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/crypto/mocks"
	backupsmock "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/backups/mocks"
	clustersmock "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/clusters/mocks"
	computemock "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/compute/mocks"
	eventsmock "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/events/mocks"
	searchmock "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/search/mocks"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/sessions"
	sessionsmock "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/sessions/mocks"
	tasksmock "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/logic/internal/tasks/mocks"
	metadbmock "a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/metadb/mocks"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/clusters"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/operations"
	"a.yandex-team.ru/cloud/mdb/mdb-internal-api/internal/models/pillars"
	pillarsecretsmocks "a.yandex-team.ru/cloud/mdb/mdb-pillar-secrets/pkg/pillarsecrets/mocks"
)

type Fixture struct {
	Ctrl           *gomock.Controller
	Operator       *clustersmock.MockOperator
	Creator        *clustersmock.MockCreator
	Restorer       *clustersmock.MockRestorer
	Reader         *clustersmock.MockReader
	Modifier       *clustersmock.MockModifier
	Backups        *backupsmock.MockBackups
	Events         *eventsmock.MockEvents
	Search         *searchmock.MockDocs
	Sessions       *sessionsmock.MockSessions
	Tasks          *tasksmock.MockTasks
	Compute        *computemock.MockCompute
	CryptoProvider *mocks.MockCrypto
	MetaDB         *metadbmock.MockBackend
	PillarSecrets  *pillarsecretsmocks.MockPillarSecretsClient
}

func (f *Fixture) ExpectBegin(ctx context.Context) {
	f.Sessions.EXPECT().Begin(ctx, gomock.Any(), gomock.Any()).Return(ctx, sessions.Session{}, nil)
	f.Sessions.EXPECT().Rollback(ctx)
}

func (f *Fixture) ExpectBeginError(ctx context.Context, err error) {
	f.Sessions.EXPECT().Begin(ctx, gomock.Any(), gomock.Any()).Return(ctx, sessions.Session{}, err)
}

func (f *Fixture) ExpectBeginWithIdempotence(ctx context.Context) {
	f.Sessions.EXPECT().BeginWithIdempotence(ctx, gomock.Any(), gomock.Any()).
		Return(ctx, sessions.Session{}, sessions.OriginalRequest{}, nil)
	f.Sessions.EXPECT().Rollback(ctx)
}

func (f *Fixture) ExpectBeginWithIdempotenceError(ctx context.Context, err error) {
	f.Sessions.EXPECT().BeginWithIdempotence(ctx, gomock.Any(), gomock.Any()).
		Return(ctx, sessions.Session{}, sessions.OriginalRequest{}, err)
}

func (f *Fixture) ExpectCommit(ctx context.Context) {
	f.Sessions.EXPECT().Commit(ctx).Return(nil)
}

func (f *Fixture) ExpectEvent(ctx context.Context) {
	f.Events.EXPECT().NewAuthentication(gomock.Any()).Return(nil)
	f.Events.EXPECT().NewAuthorization(gomock.Any()).Return(nil)
	f.Events.EXPECT().NewRequestMetadata(ctx).Return(nil)
	f.Events.EXPECT().NewEventMetadata(gomock.Any(), gomock.Any(), gomock.Any(), gomock.Any()).Return(nil, nil)
	f.Events.EXPECT().Store(ctx, gomock.Any(), gomock.Any()).Return(nil)
}

func (f *Fixture) ExpectSubClusterByRole(ctx context.Context, subCluster clusters.SubCluster, pillar pillars.Marshaler) {
	f.Reader.EXPECT().SubClusterByRole(ctx, subCluster.ClusterID, gomock.Any(), gomock.Any()).
		Return(subCluster, nil).
		Do(func(_, _, _, arg3 interface{}) {
			reflect.Indirect(reflect.ValueOf(arg3)).Set(reflect.Indirect(reflect.ValueOf(pillar)))
		})
}

func (f *Fixture) ExpectSubClusterByRoleError(ctx context.Context, cid string, err error) {
	f.Reader.EXPECT().SubClusterByRole(ctx, cid, gomock.Any(), gomock.Any()).
		Return(clusters.SubCluster{}, err)
}

func (f *Fixture) ExpectUpdateSubClusterPillar(ctx context.Context, cid, subcid string, rev int64, verifier func(pillar interface{})) {
	f.Modifier.EXPECT().UpdateSubClusterPillar(ctx, cid, subcid, rev, gomock.Any()).
		Return(nil).
		Do(func(_, _, _, _, arg4 interface{}) {
			verifier(arg4)
		})
}

func (f *Fixture) ExpectUpdatePillar(ctx context.Context, cid string, rev int64, verifier func(pillar interface{})) {
	f.Modifier.EXPECT().UpdatePillar(ctx, cid, rev, gomock.Any()).
		Return(nil).
		Do(func(_, _, _, arg3 interface{}) {
			verifier(arg3)
		})
}

func (f *Fixture) ExpectUpdateSubClusterPillarError(ctx context.Context, cid, subcid string, rev int64, err error) {
	f.Modifier.EXPECT().UpdateSubClusterPillar(ctx, cid, subcid, rev, gomock.Any()).
		Return(err)
}

func (f *Fixture) ExpectCreateTask(ctx context.Context, session sessions.Session, op operations.Operation) {
	f.Tasks.EXPECT().CreateTask(ctx, session, gomock.Any()).Return(op, nil)
}

func (f *Fixture) ExpectCreateTaskError(ctx context.Context, session sessions.Session, err error) {
	f.Tasks.EXPECT().CreateTask(ctx, session, gomock.Any()).Return(operations.Operation{}, err)
}

func NewFixture(t *testing.T) (context.Context, *Fixture) {
	ctrl := gomock.NewController(t)
	return context.Background(),
		&Fixture{
			Ctrl:           ctrl,
			Operator:       clustersmock.NewMockOperator(ctrl),
			Creator:        clustersmock.NewMockCreator(ctrl),
			Restorer:       clustersmock.NewMockRestorer(ctrl),
			Reader:         clustersmock.NewMockReader(ctrl),
			Modifier:       clustersmock.NewMockModifier(ctrl),
			Backups:        backupsmock.NewMockBackups(ctrl),
			Events:         eventsmock.NewMockEvents(ctrl),
			Search:         searchmock.NewMockDocs(ctrl),
			Sessions:       sessionsmock.NewMockSessions(ctrl),
			Tasks:          tasksmock.NewMockTasks(ctrl),
			Compute:        computemock.NewMockCompute(ctrl),
			CryptoProvider: mocks.NewMockCrypto(ctrl),
			MetaDB:         metadbmock.NewMockBackend(ctrl),
			PillarSecrets:  pillarsecretsmocks.NewMockPillarSecretsClient(ctrl),
		}
}
