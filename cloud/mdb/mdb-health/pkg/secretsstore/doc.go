/*
	This package and its implementations are placed in mdb-health ONLY because of how awful current
	integration tests model is. The environment for the tests are in mdb-health/docker-compose.yml
    and must be in the top-level package in repo. So if move this package to utils/pkg, utils will
    have to have docker-compose.yml which will be used for ALL integration tests in utils.

    So lets just keep it here for now.
*/

package secretsstore
