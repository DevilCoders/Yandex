package api

import (
	"strings"

	"a.yandex-team.ru/cloud/mdb/internal/grpcutil/interceptors"
)

var YandexCloudReadOnlyMethods = interceptors.DefaultReadOnlyMethods
var DatacloudReadOnlyMethods = interceptors.DefaultReadOnlyMethods

const (
	YandexCloudPrefix = "/yandex"
	DataCloudPrefix   = "/datacloud"
)

func IsYandexCloudMethod(method string) bool {
	return strings.HasPrefix(method, YandexCloudPrefix)
}

func IsDataCloudMethod(method string) bool {
	return strings.HasPrefix(method, DataCloudPrefix) || strings.HasPrefix(method, YandexCloudPrefix)
}
