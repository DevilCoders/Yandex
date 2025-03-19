package cmd

import (
	"bufio"
	"context"
	"fmt"
	"os"
	"strings"

	"github.com/gofrs/uuid"
	"go.uber.org/zap"

	nbs "a.yandex-team.ru/cloud/blockstore/public/sdk/go/client"
	"a.yandex-team.ru/cloud/compute/go-common/logging/ctxlog"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/client"
	"a.yandex-team.ru/cloud/compute/snapshot/snapshot/lib/globalauth"
)

const (
	adminSuffix             = "-admin"
	adminOperationProjectID = "manual-admin-operation"
)

func snapshotClient(ctx context.Context) (*client.Client, error) {
	return client.NewClient(ctx, flagEndPoint, false, globalauth.GetCredentials())
}

func createContext() context.Context {
	log, err := zap.NewDevelopment()
	if err != nil {
		panic(fmt.Sprintf("failed to create context: %v", err))
	}
	ctx := context.Background()
	ctx = ctxlog.WithLogger(ctx, log)
	return ctx
}

func nbsClient(endpoint string) (*nbs.Client, error) {
	return nbs.NewClient(&nbs.GrpcClientOpts{
		Endpoint: endpoint,
		Credentials: &nbs.ClientCredentials{
			AuthToken: providedToken,
			IAMClient: globalauth.GetCredentials(),
		},
	}, &nbs.DurableClientOpts{}, nbs.NewStderrLog(nbs.LOG_DEBUG))
}

func doAdminAction(id string) bool {
	if strings.HasSuffix(id, adminSuffix) {
		return true
	} else {
		reader := bufio.NewReader(os.Stdin)
		fmt.Printf("%s have no \"%s\" suffix, write \"yes\" to proceed: ", id, adminSuffix)
		text, _ := reader.ReadString('\n')
		return text == "yes\n"
	}
}

func ensureAdminSuffix(id string) string {
	newID := id
	if !strings.HasSuffix(id, adminSuffix) {
		newID += adminSuffix
	}
	fmt.Println("current snapshot id", id)
	return newID
}

func createTaskID(ctx context.Context) string {
	u, err := uuid.NewV4()
	if err != nil {
		panic(fmt.Sprintf("uuid error: %v", err))
	}
	taskID := snapshotID + u.String()
	fmt.Println("task id generated", taskID)
	return taskID
}
