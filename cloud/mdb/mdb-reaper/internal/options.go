package internal

import (
	chv1 "a.yandex-team.ru/cloud/mdb/datacloud/private_api/datacloud/clickhouse/v1"
	kfv1 "a.yandex-team.ru/cloud/mdb/datacloud/private_api/datacloud/kafka/v1"
	"a.yandex-team.ru/cloud/mdb/internal/compute/resmanager"
	"a.yandex-team.ru/cloud/mdb/mdb-reaper/internal/metadb"
)

type AppOption = func(*App)

func WithConfig(conf Config) AppOption {
	return func(app *App) {
		app.config = conf
	}
}

func WithResourceManager(rm resmanager.Client) AppOption {
	return func(app *App) {
		app.rm = rm
	}
}

func WithClickhouseSvc(c chv1.ClusterServiceClient, o chv1.OperationServiceClient) AppOption {
	return func(app *App) {
		app.chClusterSvc = c
		app.chOperationSvc = o
	}
}

func WithKafkaSvc(c kfv1.ClusterServiceClient, o kfv1.OperationServiceClient) AppOption {
	return func(app *App) {
		app.kfClusterSvc = c
		app.kfOperationSvc = o
	}
}

func WithMetaDB(meta metadb.MetaDB) AppOption {
	return func(app *App) {
		app.meta = meta
	}
}
