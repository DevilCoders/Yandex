package config

import (
	"encoding/json"
	"fmt"
	"os"
	"path/filepath"

	"a.yandex-team.ru/cloud/marketplace/mktctl/internal/auth"

	"github.com/mitchellh/go-homedir"
)

func getSavedFederationToken(profileName string) (*auth.Federation, error) {
	data, err := os.ReadFile(getTokenFilepath(profileName))
	if err != nil {
		return nil, err
	}

	token := &auth.Federation{}
	if err = json.Unmarshal(data, token); err != nil {
		return nil, err
	}

	return token, nil
}

const tokenPrefix = "token"
const defaultSavedTokensPath = "~/.config/mktctl/federation"

func getTokenFilepath(profileName string) string {
	savedTokensDirectory, err := homedir.Expand(defaultSavedTokensPath)
	if err != nil {
		panic(err)
	}

	return filepath.Join(savedTokensDirectory, fmt.Sprintf("%v-%v", tokenPrefix, profileName))
}

func saveToken(token *auth.Federation, profileName string) error {
	if err := makeDirectiesForTokenFilepath(); err != nil {
		return err
	}

	data, err := json.MarshalIndent(token, "", "  ")
	if err != nil {
		return err
	}

	return os.WriteFile(getTokenFilepath(profileName), data, 0700)
}

func makeDirectiesForTokenFilepath() error {
	savedTokensDirectory, err := homedir.Expand(defaultSavedTokensPath)
	if err != nil {
		return err
	}

	return os.MkdirAll(savedTokensDirectory, 0700)
}

func refreshSavedFederationToken(profileName, federationID, federationEndpoint string) error {
	token, err := auth.GetFederationToken(federationID, federationEndpoint)
	if err != nil {
		return err
	}

	return saveToken(token, profileName)
}

func mustRefreshSavedFederationToken(profileName, federationID, federationEndpoint string) {
	if err := refreshSavedFederationToken(profileName, federationID, federationEndpoint); err != nil {
		panic(err)
	}
}
