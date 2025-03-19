package client

import (
	"context"
	"fmt"
	"os"
	"time"

	"github.com/golang/protobuf/jsonpb"
	"google.golang.org/grpc/credentials"

	"a.yandex-team.ru/cloud/mdb/internal/compute/iam"
	iamgrpc "a.yandex-team.ru/cloud/mdb/internal/compute/iam/grpc"
	"a.yandex-team.ru/cloud/mdb/internal/grpcutil"
	"a.yandex-team.ru/cloud/mdb/internal/grpcutil/interceptors"
	"a.yandex-team.ru/cloud/mdb/internal/secret"
	mlock "a.yandex-team.ru/cloud/mdb/mlock/api"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
)

const (
	authMetadataKey = "authorization"
	userAgent       = "mdb-mlock-cli"
	tokenPrefix     = "Bearer "
)

// JWTConfig describes client config for getting iam tokens with JWT
type JWTConfig struct {
	Addr         string                `json:"addr" yaml:"addr"`
	ClientConfig grpcutil.ClientConfig `json:"config" yaml:"config"`
	ID           string                `json:"id" yaml:"id"`
	KeyID        secret.String         `json:"key_id" yaml:"key_id"`
	PrivateKey   secret.String         `json:"private_key" yaml:"private_key"`
}

// Config describes client config
type Config struct {
	Addr         string                `json:"addr" yaml:"addr"`
	ClientConfig grpcutil.ClientConfig `json:"config" yaml:"config"`
	Token        secret.String         `json:"token" yaml:"token"`
	JWTConfig    JWTConfig             `json:"jwt" yaml:"jwt"`
	LogLevel     log.Level             `json:"log_level" yaml:"log_level"`
}

// DefaultConfig returns default client Config
func DefaultConfig() Config {
	return Config{
		Addr:     "[::1]:30030",
		LogLevel: log.DebugLevel,
		JWTConfig: JWTConfig{
			ClientConfig: grpcutil.ClientConfig{
				Retries: interceptors.ClientRetryConfig{
					MaxTries:        3,
					PerRetryTimeout: time.Second * 5,
				},
			},
		},
		ClientConfig: grpcutil.ClientConfig{
			Security: grpcutil.SecurityConfig{
				Insecure: true,
			},
			Retries: interceptors.ClientRetryConfig{
				MaxTries:        3,
				PerRetryTimeout: time.Second * 5,
			},
		},
	}
}

// Client is a simple console client for mlock
type Client struct {
	ctx      context.Context
	mlockAPI mlock.LockServiceClient
}

// NewClient is a Client constructor
func NewClient(ctx context.Context, target, userAgent string, config grpcutil.ClientConfig,
	creds credentials.PerRPCCredentials, logger log.Logger) (*Client, error) {
	conn, err := grpcutil.NewConn(ctx, target, userAgent, config, logger, grpcutil.WithClientCredentials(creds))
	if err != nil {
		return nil, fmt.Errorf("connection to %q: %w", target, err)
	}

	return &Client{
		ctx:      ctx,
		mlockAPI: mlock.NewLockServiceClient(conn),
	}, nil
}

func getConfigCredentials(ctx context.Context, token secret.String, jwtConfig JWTConfig, logger log.Logger) (credentials.PerRPCCredentials, error) {
	if token.Unmask() != "" {
		creds := grpcutil.PerRPCCredentialsStatic{
			RequestMetadata: map[string]string{authMetadataKey: tokenPrefix + token.Unmask()},
		}
		return &creds, nil
	}

	tokenService, err := iamgrpc.NewTokenServiceClient(ctx, jwtConfig.Addr, userAgent, jwtConfig.ClientConfig, &grpcutil.PerRPCCredentialsStatic{}, logger)
	if err != nil {
		return nil, err
	}
	creds := tokenService.ServiceAccountCredentials(iam.ServiceAccount{
		ID:    jwtConfig.ID,
		KeyID: jwtConfig.KeyID.Unmask(),
		Token: []byte(jwtConfig.PrivateKey.Unmask()),
	})
	return creds, nil
}

// NewFromConfig builds a Client instance from Config
func NewFromConfig(conf Config) (*Client, error) {
	logConfig := zap.JSONConfig(conf.LogLevel)
	logConfig.OutputPaths = []string{"stderr"}
	logger, err := zap.New(logConfig)
	if err != nil {
		return nil, fmt.Errorf("unable to init logger: %w", err)
	}
	ctx := context.Background()
	creds, err := getConfigCredentials(ctx, conf.Token, conf.JWTConfig, logger)
	if err != nil {
		return nil, fmt.Errorf("unable to get auth token: %w", err)
	}
	return NewClient(ctx, conf.Addr, userAgent, conf.ClientConfig, creds, logger)
}

// Create upserts lock into mlock
func (client *Client) Create(lockID string, holder string, reason string, objects []string) int {
	_, err := client.mlockAPI.CreateLock(
		client.ctx,
		&mlock.CreateLockRequest{Id: lockID, Holder: holder, Reason: reason, Objects: objects},
	)

	if err != nil {
		fmt.Fprintf(os.Stderr, "lock %q create failed: %+v\n", lockID, err)
		return 1
	}

	fmt.Printf("lock %q created\n", lockID)
	return 0
}

// Release deletes lock from mlock
func (client *Client) Release(lockID string) int {
	_, err := client.mlockAPI.ReleaseLock(client.ctx, &mlock.ReleaseLockRequest{Id: lockID})

	if err != nil {
		fmt.Fprintf(os.Stderr, "lock %q release failed: %+v\n", lockID, err)
		return 1
	}

	fmt.Printf("lock %q released\n", lockID)
	return 0
}

// Status prints status of lock
func (client *Client) Status(lockID string) int {
	resp, err := client.mlockAPI.GetLockStatus(client.ctx, &mlock.GetLockStatusRequest{Id: lockID})

	if err != nil {
		fmt.Fprintf(os.Stderr, "get lock %q status failed: %+v\n", lockID, err)
		return 1
	}

	marshaler := jsonpb.Marshaler{EmitDefaults: true, OrigName: true}

	jsonString, err := marshaler.MarshalToString(resp)

	if err != nil {
		fmt.Fprintf(os.Stderr, "marshaling lock %q status response failed: %+v\n", lockID, err)
		return 1
	}

	fmt.Printf("lock status: %s\n", jsonString)

	if !resp.Acquired {
		return 2
	}
	return 0
}

// List prints all locks (with optional holder filter)
func (client *Client) List(holder string, limit int64) int {
	var offset int64
	for {
		resp, err := client.mlockAPI.ListLocks(
			client.ctx,
			&mlock.ListLocksRequest{Holder: holder, Offset: offset, Limit: limit},
		)
		if err != nil {
			fmt.Fprintf(os.Stderr, "list locks failed: %+v\n", err)
			return 1
		}
		for _, lock := range resp.Locks {
			marshaler := jsonpb.Marshaler{EmitDefaults: true, OrigName: true}
			jsonString, err := marshaler.MarshalToString(lock)
			if err != nil {
				fmt.Fprintf(os.Stderr, "marshaling lock list response failed: %+v\n", err)
				return 1
			}
			fmt.Printf("%s\n", jsonString)
		}
		if resp.NextOffset != 0 {
			offset = resp.NextOffset
			continue
		}
		break
	}
	return 0
}
