package tests

import (
	"a.yandex-team.ru/cloud/mdb/internal/testutil/intapi"
)

func (tctx *TestContext) postgresClusterWithName(name string) error {
	resp, err := tctx.intAPI.CreatePGCluster(intapi.CreatePostgresqlClusterRequest{
		Name: name,
	})
	if err != nil {
		return err
	}

	return tctx.completeTask(resp)
}

func (tctx *TestContext) mysqlClusterWithName(name string) error {
	resp, err := tctx.intAPI.CreateCluster("mysql", intapi.CreateClusterRequest{Name: name})
	if err != nil {
		return err
	}

	return tctx.completeTask(resp)
}

func (tctx *TestContext) weInCreatePostgresClusterWithNameUsingToken(folder, name, token string) error {
	resp, err := tctx.intAPI.CreatePGCluster(intapi.CreatePostgresqlClusterRequest{
		FolderID: folder,
		Token:    token,
		Name:     name,
	})
	if err != nil {
		return err
	}

	return tctx.completeTask(resp)
}

func (tctx *TestContext) completeTask(resp intapi.OperationResponse) error {
	return tctx.worker.AcquireAndSuccessfullyCompleteTask(tctx.TC.Context(), resp.ID)
}
