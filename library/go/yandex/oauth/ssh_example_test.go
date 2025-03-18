package oauth_test

import (
	"context"
	"fmt"

	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
	"a.yandex-team.ru/library/go/yandex/oauth"
)

func ExampleGetTokenBySSH() {
	const (
		clientID     = "d27043e1166e48a5850894750254fd93"
		clientSecret = "3046662e1650427190e71da509f7d5d0"
	)

	logger, err := zap.New(zap.ConsoleConfig(log.DebugLevel))
	if err != nil {
		panic(err)
	}

	fmt.Printf("Try to exchange ssh key to OAuth token for '%s'\n", clientID)
	token, err := oauth.GetTokenBySSH(
		context.TODO(),
		clientID, clientSecret,
		oauth.WithLogger(logger),
	)
	if err != nil {
		fmt.Printf("fail: %s\n", err.Error())
		return
	}

	fmt.Printf("oauth token: %s\n", token)
}

func ExampleGetTokenBySSH_custom() {
	const (
		clientID     = "d27043e1166e48a5850894750254fd93"
		clientSecret = "3046662e1650427190e71da509f7d5d0"
	)

	logger, err := zap.New(zap.ConsoleConfig(log.DebugLevel))
	if err != nil {
		panic(err)
	}

	keyring, err := oauth.NewSSHFileKeyring("/etc/ssh.key")
	if err != nil {
		panic(err)
	}
	defer func() { _ = keyring.Close() }()

	fmt.Printf("Try to exchange ssh key to OAuth token for '%s'\n", clientID)
	token, err := oauth.GetTokenBySSH(
		context.TODO(),
		clientID, clientSecret,
		oauth.WithLogger(logger),
		oauth.WithSSHKeyring(keyring),
		oauth.WithUserLogin("real-login"),
	)
	if err != nil {
		fmt.Printf("fail: %s\n", err.Error())
		return
	}

	fmt.Printf("oauth token: %s\n", token)
}

func ExampleGetTokenBySSH_multipleKeyrings() {
	const (
		clientID     = "d27043e1166e48a5850894750254fd93"
		clientSecret = "3046662e1650427190e71da509f7d5d0"
	)

	logger, err := zap.New(zap.ConsoleConfig(log.DebugLevel))
	if err != nil {
		panic(err)
	}

	keyKeyring, err := oauth.NewSSHFileKeyring("/etc/ssh.key")
	if err != nil {
		panic(err)
	}
	defer func() { _ = keyKeyring.Close() }()

	agentKeyring, err := oauth.NewSSHAgentKeyring(
		"SHA256:X2m3tJc1JGNEqmMue6oUYEdTirh3jqzmwc/cdipa9Nk",
		"SHA256:cRyZYT2uA6cM8cbDj1xY17SXmDWnnkFmByD+GBOsToY",
	)
	if err != nil {
		panic(err)
	}
	defer func() { _ = agentKeyring.Close() }()

	fmt.Printf("Try to exchange ssh key to OAuth token for '%s'\n", clientID)
	token, err := oauth.GetTokenBySSH(
		context.Background(),
		clientID, clientSecret,
		oauth.WithLogger(logger),
		oauth.WithSSHKeyring(keyKeyring, agentKeyring),
	)
	if err != nil {
		fmt.Printf("fail: %s\n", err.Error())
		return
	}

	fmt.Printf("oauth token: %s\n", token)
}
