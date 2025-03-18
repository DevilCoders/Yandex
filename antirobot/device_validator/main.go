package main

import (
	"encoding/hex"
	"log"
	"net/http"
	"os"
	"strings"

	"a.yandex-team.ru/devtools/distbuild/go/unifiedagent"
	yazap "a.yandex-team.ru/library/go/core/log/zap"
	"github.com/golang-jwt/jwt/v4"
	"go.uber.org/zap"
	"golang.org/x/sys/unix"
)

type Env struct {
	Args               *Args
	AppIDToConfig      map[string]*ServiceConfig
	Stats              Stats
	NonceKey           []byte
	TokenSigningKeyID  string
	TokenSigningMethod jwt.SigningMethod
	TokenSigningKey    []byte
	UnifiedAgentClient unifiedagent.Client
}

func main() {
	_ = unix.Mlockall(unix.MCL_FUTURE | unix.MCL_CURRENT)

	var err error
	args := ParseArgs(os.Args[1:])
	env := &Env{Args: &args}

	serviceConfig, err := ParseServiceConfig(args.ServiceConfig)
	if err != nil {
		log.Fatalln("Failed to parse the service config:", err)
	}

	env.AppIDToConfig = make(map[string]*ServiceConfig)
	for _, serviceConfigEntry := range serviceConfig {
		for _, appID := range serviceConfigEntry.AppIDs {
			env.AppIDToConfig[appID] = &serviceConfigEntry
		}
	}

	appIDs := []string{}
	if len(args.AppIDs) > 0 {
		appIDs, err = ReadLines(args.AppIDs)
		if err != nil {
			log.Fatalln("Failed to read list of app ids:", appIDs)
		}
	}

	env.Stats = NewStats(appIDs)

	nonceKeyData, err := os.ReadFile(args.NonceKey)
	if err != nil {
		log.Fatalln("Failed to read nonce key file:", err)
	}

	env.NonceKey, err = hex.DecodeString(string(nonceKeyData))
	if err != nil {
		log.Fatalln("Invalid nonce key file:", err)
	}

	if len(env.NonceKey) != NonceHMACKeySize {
		log.Fatalln("Invalid nonce key size")
	}

	tokenSigningData, err := os.ReadFile(args.TokenSigningKey)
	if err != nil {
		log.Fatalln("Failed to read token signing key file:", err)
	}

	tokenSigningDataTokens := strings.Split(strings.TrimSpace(string(tokenSigningData)), ":")

	if len(tokenSigningDataTokens) != 3 {
		log.Fatalln("Invalid token signing key format (expected KEY_ID:ALGO:KEY)")
	}

	env.TokenSigningKeyID = tokenSigningDataTokens[0]
	env.TokenSigningMethod = jwt.GetSigningMethod(tokenSigningDataTokens[1])
	if env.TokenSigningMethod == nil {
		log.Fatalln("Invalid token signing method")
	}

	env.TokenSigningKey, err = hex.DecodeString(tokenSigningDataTokens[2])
	if err != nil {
		log.Fatalln("Invalid token signing key:", err)
	}

	if len(args.UnifiedAgentURI) > 0 {
		unifiedAgentLoggerConfig := zap.NewProductionConfig()
		unifiedAgentLoggerConfig.Level.SetLevel(zap.WarnLevel)
		unifiedAgentLoggerConfig.Encoding = "console"
		unifiedAgentLogger, err := yazap.New(unifiedAgentLoggerConfig)
		if err != nil {
			log.Fatalln("Failed to create unified agent logger:", err)
		}

		env.UnifiedAgentClient = unifiedagent.NewRetriableClient(
			args.UnifiedAgentURI,
			unifiedAgentLogger,
		)

		err = env.UnifiedAgentClient.Connect("")
		if err != nil {
			log.Fatalln("Failed to connect unified agent client:", err)
		}

		defer func() {
			err := env.UnifiedAgentClient.Close()
			if err != nil {
				log.Fatalln("Failed to close unified agent client:", err)
			}
		}()
	}

	apiServeMux := http.NewServeMux()

	err = InstallAndroidAPI(env, apiServeMux)
	if err != nil {
		log.Fatalln("Failed to set up Android API:", err)
	}

	err = InstallIOSAPI(env, apiServeMux)
	if err != nil {
		log.Fatalln("Failed to set up iOS API:", err)
	}

	go func() {
		log.Fatal(http.ListenAndServe(args.Addr, apiServeMux))
	}()

	statsServeMux := http.NewServeMux()
	statsServeMux.HandleFunc("/stats", makeHandleStats(&env.Stats))

	log.Fatal(http.ListenAndServe(args.StatsAddr, statsServeMux))
}

func makeHandleStats(stats *Stats) func(writer http.ResponseWriter, request *http.Request) {
	return func(writer http.ResponseWriter, request *http.Request) {
		SetJSONContentType(writer)
		_ = stats.Dump(writer)
	}
}
