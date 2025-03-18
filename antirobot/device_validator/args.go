package main

import (
	"flag"
	"strings"
)

type Args struct {
	ServiceConfig                string
	Addr                         string
	StatsAddr                    string
	NonceKey                     string
	TokenSigningKey              string
	MaxTokenAgeMs                uint64
	AppIDs                       string
	APKCertFingerprints          string
	MaxAttestationAgeMs          uint64
	DeviceCheckAPI               string
	DeviceCheckKey               string
	DeviceCheckRefreshIntervalMs uint64
	UnifiedAgentURI              string
}

func ParseArgs(inputArgs []string) (args Args) {
	flagSet := flag.NewFlagSet("device_validator", flag.PanicOnError)

	flagSet.StringVar(&args.ServiceConfig, "service-config", "", "path to service config")

	flagSet.StringVar(&args.Addr, "addr", ":8080", "address to listen to for validation API")

	flagSet.StringVar(&args.StatsAddr, "stats-addr", ":8081", "address to listen for stats API")

	flagSet.StringVar(
		&args.NonceKey, "nonce-key", "",
		"path to nonce key file",
	)

	flagSet.StringVar(
		&args.TokenSigningKey, "token-signing-key", "",
		"path to token signing key file",
	)

	flagSet.Uint64Var(
		&args.MaxTokenAgeMs, "max-token-age-ms", 0,
		"max token age in milliseconds",
	)

	flagSet.StringVar(
		&args.AppIDs, "app-ids", "",
		"path to file with line-separated app ids",
	)

	flagSet.StringVar(
		&args.APKCertFingerprints, "apk-cert-fingerprints", "",
		"path to file with line-separated APK cert fingerprints, cert checking is disabled if not set",
	)

	flagSet.Uint64Var(
		&args.MaxAttestationAgeMs, "max-attestation-age-ms", 3000,
		"max SafetyNet attestation age in milliseconds",
	)

	flagSet.StringVar(
		&args.DeviceCheckAPI, "device-check-api", "https://api.development.devicecheck.apple.com",
		"DeviceCheck API prefix",
	)

	flagSet.StringVar(
		&args.DeviceCheckKey, "device-check-key", "",
		"path DeviceCheck key file",
	)

	flagSet.Uint64Var(
		&args.DeviceCheckRefreshIntervalMs, "device-check-refresh-interval-ms", 50*60*1000,
		"DeviceCheck server signature refresh interval in milliseconds",
	)

	flagSet.StringVar(
		&args.UnifiedAgentURI, "unified-agent-uri", "",
		"unified agent GRPC URI",
	)

	// Shouldn't return an error since we use flag.PanicOnError.
	_ = flagSet.Parse(inputArgs)

	return
}

func parseCommaSeparatedList(s string) (trimmedTokens []string) {
	if len(s) == 0 {
		return
	}

	tokens := strings.Split(s, ",")
	trimmedTokens = make([]string, len(tokens))

	for i, token := range tokens {
		trimmedTokens[i] = strings.TrimSpace(token)
	}

	return
}
