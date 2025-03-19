package functest

import (
	"context"
	"time"

	"github.com/DATA-DOG/godog"
	"github.com/jmoiron/sqlx"

	"a.yandex-team.ru/cloud/mdb/internal/godogutil"
	"a.yandex-team.ru/cloud/mdb/internal/logbroker/writer/dummy"
	"a.yandex-team.ru/cloud/mdb/internal/monrun"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/pgutil"
	"a.yandex-team.ru/cloud/mdb/internal/sqlutil/pgutil/recipeconfig"
	"a.yandex-team.ru/cloud/mdb/mdb-search-producer/internal/app"
	mdbpg "a.yandex-team.ru/cloud/mdb/mdb-search-producer/internal/metadb/pg"
	"a.yandex-team.ru/cloud/mdb/mdb-search-producer/internal/monitoring"
	"a.yandex-team.ru/cloud/mdb/mdb-search-producer/internal/producer"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
	"a.yandex-team.ru/library/go/core/xerrors"
	"a.yandex-team.ru/library/go/x/yandex/hasql/tracers"
)

func (s *steps) emptySearchQueue() error {
	db, err := s.getDB()
	if err != nil {
		return err
	}
	result, err := db.Exec("TRUNCATE dbaas.search_queue")
	if err != nil {
		return err
	}
	_, err = result.RowsAffected()
	return err
}

func (s *steps) documentsInSearchQueue(documents int) error {
	db, err := s.getDB()
	if err != nil {
		return err
	}
	for docNum := 0; docNum < documents; docNum++ {
		result, err := db.Exec(
			"INSERT INTO dbaas.search_queue (doc) VALUES (jsonb_build_object('doc', $1))",
			docNum,
		)
		if err == nil {
			_, err = result.RowsAffected()
		}
		if err != nil {
			return xerrors.Errorf("fail to insert new doc: %w", err)
		}
	}
	return nil
}

func makeAppCfg(metaCfg pgutil.Config) app.Config {
	cfg := app.DefaultConfig()
	cfg.MetaDB = metaCfg
	cfg.Producer.Sleeps.From.Duration = time.Millisecond
	cfg.Producer.Sleeps.To.Duration = time.Second
	cfg.Producer.IterationsLimit = 3

	return cfg
}

func (s *steps) iRunOneProducer() error {
	appCfg := makeAppCfg(s.mdbCfg)

	cluster, err := pgutil.NewCluster(appCfg.MetaDB, sqlutil.WithTracer(tracers.Log(s.L)))
	if err != nil {
		return err
	}
	pr, err := producer.New(s.L, mdbpg.New(cluster, s.L), dummy.New(), appCfg.Producer)
	if err != nil {
		return err
	}

	return pr.Run(s.TC.Context())
}

func (s *steps) iRunSearchQueueMonitoring(warnStr, critStr string) error {
	logger, err := zap.New(zap.KVConfig(log.DebugLevel))
	if err != nil {
		return xerrors.Errorf("Unable to init logger: %w", err)
	}
	warn, err := time.ParseDuration(warnStr)
	if err != nil {
		return xerrors.Errorf("Bad warn format: %w", err)
	}
	crit, err := time.ParseDuration(critStr)
	if err != nil {
		return xerrors.Errorf("Bad crit format: %w", err)
	}

	monCtx, monCancel := context.WithCancel(s.TC.Context())

	// Use addition cancel, cause monitoring create new pg cluster.
	// We should stop it.
	defer monCancel()
	mon := monitoring.NewFromAppConfig(logger, makeAppCfg(s.mdbCfg))
	res := mon.CheckDocumentsAge(monCtx, warn, crit)
	s.monitoringResult = &res
	return nil
}

func (s *steps) monitoringReturns(codeStr string) error {
	monrunCode, err := monrun.ResultCodeFromString(codeStr)
	if err != nil {
		return err
	}
	if s.monitoringResult == nil {
		return xerrors.New("There are no monitoring results")
	}

	if s.monitoringResult.Code != monrunCode {
		return xerrors.Errorf(
			"Monitoring return %s. Expected %s. Message: %s",
			s.monitoringResult.Code,
			monrunCode,
			s.monitoringResult.Message,
		)
	}
	return nil
}

func (s *steps) getUnsetDocumentsCount() (int64, error) {
	var unsentCount int64

	err := s.onMaster(func(db *sqlx.DB) error {
		row := db.QueryRow("SELECT count(*) FROM dbaas.search_queue WHERE sent_at IS NULL")
		if err := row.Scan(&unsentCount); err != nil {
			return err
		}
		return nil
	})

	return unsentCount, err
}

func (s *steps) inSearchQueueThereAreUnsentDocuments(count int64) error {
	unsentCount, err := s.getUnsetDocumentsCount()
	if err != nil {
		return err
	}
	if unsentCount != count {
		return xerrors.Errorf("Expect %d unset documents, but %d found", count, unsentCount)
	}
	return nil
}

func (s *steps) inSearchQueueThereAreNoUnsentDocuments() error {
	unsentCount, err := s.getUnsetDocumentsCount()
	if err != nil {
		return err
	}
	if unsentCount != 0 {
		return xerrors.Errorf("Expect 0 unset documents, by %d unsent documents found", unsentCount)
	}
	return nil
}

func (s *steps) iSleep(durStr string) error {
	dur, err := time.ParseDuration(durStr)
	if err != nil {
		return err
	}
	time.Sleep(dur)
	return nil
}

func contextInitializer(tc *godogutil.TestContext, s *godog.Suite) {
	steps, err := newSteps(tc)
	if err != nil {
		panic(err)
	}

	s.Step(`^empty search_queue$`, steps.emptySearchQueue)
	s.Step(`^"([^"]*)" documents in search_queue$`, steps.documentsInSearchQueue)
	s.Step(`^I add "([^"]*)" documents to the search_queue$`, steps.documentsInSearchQueue)
	s.Step(`^in search_queue there are no unsent documents$`, steps.inSearchQueueThereAreNoUnsentDocuments)
	s.Step(`^in search_queue there are "(\d+)" unsent documents$`, steps.inSearchQueueThereAreUnsentDocuments)

	s.Step(`^I sleep "([^"]*)"$`, steps.iSleep)

	s.Step(`^I run mdb-search-producer$`, steps.iRunOneProducer)

	s.Step(`^I run search-queue-documents-age with warn="([^"]+)" crit="([^"]+)"$`, steps.iRunSearchQueueMonitoring)
	s.Step(`^monitoring returns "([^"]+)"$`, steps.monitoringReturns)
}

type steps struct {
	L                log.Logger
	TC               *godogutil.TestContext
	mdb              *sqlutil.Cluster
	mdbCfg           pgutil.Config
	monitoringResult *monrun.Result
}

func (s *steps) getDB() (*sqlx.DB, error) {
	master := s.mdb.Primary()
	if master == nil {
		return nil, xerrors.New("master not available at that moment")
	}
	return master.DBx(), nil
}

func (s *steps) onMaster(handler func(db *sqlx.DB) error) error {
	db, err := s.getDB()
	if err != nil {
		return err
	}
	return handler(db)
}

func newSteps(tc *godogutil.TestContext) (*steps, error) {
	L, err := zap.New(zap.KVConfig(log.DebugLevel))
	if err != nil {
		return nil, xerrors.Errorf("failed to init logger: %w", err)
	}

	mdbCfg, err := recipeconfig.New("metadb", "dbaas_metadb", "")
	if err != nil {
		return nil, err
	}

	mdb, err := pgutil.NewCluster(mdbCfg, sqlutil.WithTracer(tracers.Log(L)))
	if err != nil {
		return nil, xerrors.Errorf("failed to initialize metadb backend: %w", err)
	}

	if _, err = mdb.WaitForPrimary(context.Background()); err != nil {
		_ = mdb.Close()
		return nil, xerrors.Errorf("no master: %w", err)
	}

	return &steps{mdb: mdb, mdbCfg: mdbCfg, TC: tc, L: L}, nil
}
