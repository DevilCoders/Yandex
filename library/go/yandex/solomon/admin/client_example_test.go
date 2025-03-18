package admin_test

import (
	"context"
	"fmt"
	"os"

	"a.yandex-team.ru/library/go/yandex/oauth"
	"a.yandex-team.ru/library/go/yandex/solomon/admin"
)

func Example() {
	oauthToken, err := oauth.GetTokenBySSH(context.Background(), admin.SolomonCLIID, admin.SolomonCLISecret)
	if err != nil {
		panic(err)
	}

	client := admin.New("yt", oauthToken)

	clusters, err := client.ListClusters(context.Background())
	if err != nil {
		panic(err)
	}

	for _, cluster := range clusters {
		fmt.Fprintf(os.Stderr, "%s %s\n", cluster.ID, cluster.Name)
	}
}
