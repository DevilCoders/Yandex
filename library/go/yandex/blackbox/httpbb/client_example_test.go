package httpbb_test

import (
	"context"
	"fmt"

	"a.yandex-team.ru/library/go/yandex/blackbox"
	"a.yandex-team.ru/library/go/yandex/blackbox/httpbb"
)

func ExampleClient_OAuth() {
	// we use intranet blackbox
	bb, err := httpbb.NewIntranet()
	if err != nil {
		panic(err)
	}

	response, err := bb.OAuth(context.TODO(), blackbox.OAuthRequest{
		OAuthToken: "<token-from-header>",
		UserIP:     "<user-ip>",
		Scopes:     []string{"myapp:all", "other:scopes"},
	})
	if err != nil {
		if blackbox.IsUnauthorized(err) {
			// TODO: redirect to oauth authorize page
		}
		panic(err)
	}

	fmt.Printf("Passport user ID: %d\n", response.User.ID)
}

func ExampleClient_SessionID() {
	// we use intranet blackbox
	bb, err := httpbb.NewIntranet()
	if err != nil {
		panic(err)
	}

	response, err := bb.SessionID(context.TODO(), blackbox.SessionIDRequest{
		SessionID: "<Session_id cookie>",
		UserIP:    "<user-ip>",
		Host:      "myapp.yandex-team.ru",
	})
	if err != nil {
		if blackbox.IsUnauthorized(err) {
			// TODO: redirect to passport login page
		}
		panic(err)
	}

	fmt.Printf("Passport user ID: %d\n", response.User.ID)
}

func ExampleClient_MultiSessionID() {
	// we use intranet blackbox
	bb, err := httpbb.NewIntranet()
	if err != nil {
		panic(err)
	}

	response, err := bb.MultiSessionID(context.TODO(), blackbox.MultiSessionIDRequest{
		SessionID: "<Session_id cookie>",
		UserIP:    "<user-ip>",
		Host:      "myapp.yandex-team.ru",
	})
	if err != nil {
		if blackbox.IsUnauthorized(err) {
			// TODO: redirect to passport login page
		}
		panic(err)
	}

	if defaultUser, err := response.DefaultUser(); err == nil {
		fmt.Printf("default session user: %d (%s)\n", defaultUser.ID, defaultUser.Login)
	}

	fmt.Println("users in session:")
	for _, user := range response.Users {
		fmt.Printf("%d (%s)\n", user.ID, user.Login)
	}
}
