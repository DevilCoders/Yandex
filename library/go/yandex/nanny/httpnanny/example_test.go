package httpnanny

import (
	"context"
	"fmt"

	"a.yandex-team.ru/library/go/yandex/nanny"
)

func ExampleClient_ListServices() {
	ctx := context.Background()

	// Create client.
	c, err := New(WithToken("your-token"))
	if err != nil {
		panic(err)
	}

	// List services for specific category.
	services, err := c.ListServices(ctx, nanny.ListOptions{Category: "/yt/hahn", ExcludeRuntimeAttrs: true})
	if err != nil {
		panic(err)
	}

	for _, s := range services {
		fmt.Println("found", s.ID)
	}
}

func ExampleClient_UpdateRuntimeAttrs() {
	ctx := context.Background()

	const myServiceID = "yt_banach_nodes"

	// Create client.
	c, err := New(WithToken("your-token"))
	if err != nil {
		panic(err)
	}

	// Get runtime attributes and update config.
	runtime, err := c.GetRuntimeAttrs(ctx, myServiceID)
	if err != nil {
		panic(err)
	}

	runtime.Attrs.Resources.UpdateStaticFile("config.json", `{"my": "config", "foo": "bar"}`)
	_, err = c.UpdateRuntimeAttrs(ctx, myServiceID, runtime.Attrs, runtime.UpdateWithComment("update config.json"))
	if err != nil {
		panic(err)
	}
}
