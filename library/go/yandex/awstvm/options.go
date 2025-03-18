package awstvm

import (
	"a.yandex-team.ru/library/go/yandex/tvm"
)

type Option func(*AuthProvider)

func WithAccessKeyID(keyID string) Option {
	return func(provider *AuthProvider) {
		provider.accessKeyID = keyID
	}
}

func WithTvmClientID(tvmID tvm.ClientID) Option {
	return func(provider *AuthProvider) {
		provider.tvmID = tvmID
	}
}

func WithProviderName(name string) Option {
	return func(provider *AuthProvider) {
		provider.name = name
	}
}

func WithSecretAccessKey(accessKey string) Option {
	return func(provider *AuthProvider) {
		provider.secretAccessKey = accessKey
	}
}
