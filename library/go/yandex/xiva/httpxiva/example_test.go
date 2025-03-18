package httpxiva_test

import (
	"context"
	"fmt"

	"a.yandex-team.ru/library/go/yandex/xiva"
	"a.yandex-team.ru/library/go/yandex/xiva/httpxiva"
)

func ExampleClient_GetWebsocketURL() {
	ctx := context.Background()
	userID := "user-id"

	client, err := httpxiva.NewClient(
		httpxiva.WithHTTPHost("https://push-sandbox.yandex.ru"),
		httpxiva.WithServiceName("test-service"),
		httpxiva.WithTokens("listen-token", "send-token"),
	)
	if err != nil {
		panic(err)
	}

	filter := xiva.NewFilter().
		AppendKeyEquals("device_id", []string{"123"}, xiva.SendBright).
		AppendAction(xiva.Skip)
	url, _ := client.GetWebsocketURL(ctx, userID, "test-session", "my-application", filter)
	fmt.Println(url)
}

func ExampleClient_SendEvent() {
	ctx := context.Background()
	userID := "user-id"

	client, err := httpxiva.NewClient(
		httpxiva.WithHTTPHost("https://push-sandbox.yandex.ru"),
		httpxiva.WithServiceName("test-service"),
		httpxiva.WithTokens("listen-token", "send-token"),
	)
	if err != nil {
		panic(err)
	}

	err = client.SendEvent(ctx, userID, "test-event", xiva.Event{
		Payload: "json-serializable-data",
		Keys:    map[string]string{"device_id": "123"},
	})
	if err != nil {
		panic(err)
	}
}
