package token

import (
	"bufio"
	"fmt"
	"strings"

	"a.yandex-team.ru/cloud/mdb/internal/cli"
	"a.yandex-team.ru/cloud/mdb/mdb-cli/pkg/common/browser"
)

const beforeBrowserOpenNote = `
You are need to provide a token for the %s.

Press Enter to open a browser.
`
const tokenGetFailedNote = `
Failed to get a %s token: %s

You should get it '%s'.
And store it in config.
`

type Description struct {
	// URL where user should get token
	URL  string
	Name string
}

func getToken(env *cli.Env, desc Description) (string, error) {
	in := bufio.NewReader(env.RootCmd.Cmd.InOrStdin())
	env.RootCmd.Cmd.Printf(beforeBrowserOpenNote, desc.Name)
	_, _, _ = in.ReadLine()
	if err := browser.OpenURL(desc.URL); err != nil {
		return "", fmt.Errorf("open a browser: %w", err)
	}
	env.RootCmd.Cmd.Print("Please input token: ")
	token, err := in.ReadString('\n')
	if err != nil {
		return "", fmt.Errorf("read a token: %w", err)
	}
	return strings.TrimSpace(token), nil
}

func Get(env *cli.Env, desc Description) (string, error) {
	t, err := getToken(env, desc)
	if err != nil {
		env.RootCmd.Cmd.Printf(tokenGetFailedNote, desc.Name, err, desc.URL)
	}
	return t, err
}
