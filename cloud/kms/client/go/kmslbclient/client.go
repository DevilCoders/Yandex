package kmslbclient

import (
	"context"
	"crypto/tls"

	"google.golang.org/grpc"
	"google.golang.org/grpc/credentials"

	"a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/kms/v1"
)

type KMSClient interface {
	// Encrypts plaintext with given keyID, aad may be nil.
	Encrypt(ctx context.Context, keyID string, plaintext []byte, aad []byte, creds *CallCredentials) (ciphertext []byte, err error)
	// Decrypts ciphertext with given keyID, aad may be nil.
	Decrypt(ctx context.Context, keyID string, ciphertext []byte, aad []byte, creds *CallCredentials) (plaintext []byte, err error)
	// Closes the client.
	Close()
}

type client struct {
	options *ClientOptions

	resolver Resolver

	balancer Balancer

	retryPolicy RetryPolicy
}

func (c *client) Encrypt(ctx context.Context, keyID string, plaintext []byte, aad []byte, callCreds *CallCredentials) (ciphertext []byte, err error) {
	ret, err := CallWithRetry(ctx, c.balancer, c.retryPolicy, func(conn *grpc.ClientConn) (interface{}, error) {
		request := kms.SymmetricEncryptRequest{
			KeyId:      keyID,
			AadContext: aad,
			Plaintext:  plaintext,
		}
		sc := kms.NewSymmetricCryptoServiceClient(conn)
		ctx, err = appendAuthorization(ctx, c.options.TokenProvider, callCreds)
		if err != nil {
			return nil, err
		}
		return sc.Encrypt(ctx, &request)
	})
	if err != nil {
		return nil, err
	}
	return ret.(*kms.SymmetricEncryptResponse).Ciphertext, err
}

func (c *client) Decrypt(ctx context.Context, keyID string, ciphertext []byte, aad []byte, callCreds *CallCredentials) (plaintext []byte, err error) {
	ret, err := CallWithRetry(ctx, c.balancer, c.retryPolicy, func(conn *grpc.ClientConn) (interface{}, error) {
		request := kms.SymmetricDecryptRequest{
			KeyId:      keyID,
			AadContext: aad,
			Ciphertext: ciphertext,
		}
		sc := kms.NewSymmetricCryptoServiceClient(conn)
		ctx, err = appendAuthorization(ctx, c.options.TokenProvider, callCreds)
		if err != nil {
			return nil, err
		}
		return sc.Decrypt(ctx, &request)
	})
	if err != nil {
		return nil, err
	}
	return ret.(*kms.SymmetricDecryptResponse).Plaintext, err
}

func (c *client) Close() {
	c.resolver.Close()
	c.balancer.Close()
}

// Only one of TargetAddrs or DiscoveryAddr must be specified.
type ClientAddress struct {
	TargetAddrs   []string
	DiscoveryAddr string
}

func NewLBSymmetricCryptoClient(address *ClientAddress, options *ClientOptions) (KMSClient, error) {
	options = fillDefaultOptions(options)
	options.Logger.Debugf("Creating client for targets %+v with options %+v", address, options)

	var resolver Resolver
	if address.DiscoveryAddr != "" {
		var err error
		resolver, err = NewDiscoveryResolver(address.DiscoveryAddr, "kms", options)
		if err != nil {
			return nil, err
		}
	} else {
		resolver = NewPassthroughResolver(address.TargetAddrs)
	}
	var dialOptions []grpc.DialOption
	if options.Plaintext {
		dialOptions = append(dialOptions, grpc.WithInsecure())
	} else {
		dialOptions = append(dialOptions,
			grpc.WithTransportCredentials(credentials.NewTLS(&tls.Config{
				ServerName: options.TLSTarget,
			})))
	}
	balancer := NewPingingBalancer(options.Balancer, dialOptions, resolver, options.Logger)
	retryPolicy := NewExponentialRetryPolicy(options.Retry)
	c := &client{
		options:     options,
		resolver:    resolver,
		balancer:    balancer,
		retryPolicy: retryPolicy,
	}
	return c, nil
}
