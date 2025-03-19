package rewriters

import (
	"net/http"

	"a.yandex-team.ru/library/go/core/log"
)

func Build(r *http.Request, clusterID, service, dataplaneDomain, uiProxyDomain string, logger log.Logger) Rewriter {
	gr := GenericRewriter{
		Request:         r,
		ClusterID:       clusterID,
		DataplaneDomain: dataplaneDomain,
		UIProxyDomain:   uiProxyDomain,
		logger:          logger,
	}
	switch service {
	case "hdfs":
		return &HDFSRewriter{gr}
	case "webhdfs":
		return &WebHDFSRewriter{gr}
	case "hive":
		return &HiveRewriter{gr}
	default:
		return &gr
	}
}
