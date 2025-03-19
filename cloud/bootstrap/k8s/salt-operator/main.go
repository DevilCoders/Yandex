/*
Copyright 2022.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

package main

import (
	"flag"
	"os"
	"time"

	ydbZap "github.com/ydb-platform/ydb-go-sdk-zap"
	"github.com/ydb-platform/ydb-go-sdk/v3"
	"github.com/ydb-platform/ydb-go-sdk/v3/trace"
	yc "github.com/ydb-platform/ydb-go-yc-metadata"
	"k8s.io/apimachinery/pkg/runtime"
	utilruntime "k8s.io/apimachinery/pkg/util/runtime"
	clientgoscheme "k8s.io/client-go/kubernetes/scheme"
	// Import all Kubernetes client auth plugins (e.g. Azure, GCP, OIDC, etc.)
	// to ensure that exec-entrypoint and run can make use of them.
	_ "k8s.io/client-go/plugin/pkg/client/auth"
	ctrl "sigs.k8s.io/controller-runtime"
	"sigs.k8s.io/controller-runtime/pkg/healthz"
	"sigs.k8s.io/controller-runtime/pkg/log/zap"
	//+kubebuilder:scaffold:imports
	"sigs.k8s.io/controller-runtime/pkg/metrics"

	bootstrapv1alpha1 "a.yandex-team.ru/cloud/bootstrap/k8s/salt-operator/api/v1alpha1"
	"a.yandex-team.ru/cloud/bootstrap/k8s/salt-operator/controllers"
	ydblock "a.yandex-team.ru/cloud/bootstrap/ydb-lock"
)

var (
	scheme   = runtime.NewScheme()
	setupLog = ctrl.Log.WithName("setup")
)

func init() {
	utilruntime.Must(clientgoscheme.AddToScheme(scheme))

	utilruntime.Must(bootstrapv1alpha1.AddToScheme(scheme))
	//+kubebuilder:scaffold:scheme
}

func main() {
	var metricsAddr string
	var enableLeaderElection bool
	var probeAddr string
	var configFile string
	var err error
	flag.StringVar(&configFile, "config", "",
		"The controller will load its initial configuration from this file. "+
			"Omit this flag to use the default configuration values. "+
			"Command-line flags override configuration from this file.")
	flag.StringVar(&metricsAddr, "metrics-bind-address", "", "The address the metric endpoint binds to.")
	flag.StringVar(&probeAddr, "health-probe-bind-address", "", "The address the probe endpoint binds to.")
	flag.BoolVar(&enableLeaderElection, "leader-elect", false,
		"Enable leader election for controller manager. "+
			"Enabling this will ensure there is only one active controller manager.")
	opts := zap.Options{
		Development: true,
	}
	opts.BindFlags(flag.CommandLine)
	flag.Parse()

	zapOpts := zap.UseFlagOptions(&opts)
	ctrl.SetLogger(zap.New(zapOpts))
	zapLogger := zap.NewRaw(zapOpts)

	syncPeriod := time.Hour * time.Duration(6)
	options := ctrl.Options{
		Scheme:                 scheme,
		MetricsBindAddress:     metricsAddr,
		Port:                   9443,
		HealthProbeBindAddress: probeAddr,
		LeaderElection:         enableLeaderElection,
		SyncPeriod:             &syncPeriod,
	}
	if configFile != "" {
		options, err = options.AndFrom(ctrl.ConfigFile().AtPath(configFile))
		if err != nil {
			setupLog.Error(err, "unable to load the config file")
			os.Exit(1)
		}
	}
	mgr, err := ctrl.NewManager(ctrl.GetConfigOrDie(), options)
	if err != nil {
		setupLog.Error(err, "unable to start manager")
		os.Exit(1)
	}

	ctx := ctrl.SetupSignalHandler()
	ydbOpts := []ydb.Option{
		ydbZap.WithTraces(
			zapLogger,
			trace.DetailsAll,
		),
		yc.WithInternalCA(),
	}
	metadataURL := os.Getenv("YC_METADATA_URL")
	if metadataURL != "" {
		ydbOpts = append(ydbOpts, yc.WithCredentials(
			yc.WithURL(metadataURL),
		))
	}
	ydbDatabase := os.Getenv("YDB_DATABASE")
	if ydbDatabase != "" {
		ydbOpts = append(ydbOpts, ydb.WithDatabase(
			ydbDatabase,
		))
	}
	db, err := ydb.Open(
		ctx,
		os.Getenv("YDB_ENDPOINT"),
		ydbOpts...,
	)
	if err != nil {
		setupLog.Error(err, "unable to open ydb connection")
		os.Exit(1)
	}
	ylc := ydblock.NewClient(
		db,
		os.Getenv("YDBLOCK_PREFIX"),
	)
	ylc.Logger = zapLogger
	ylc.MetricsRegistry = metrics.Registry
	err = ylc.Init(ctx)
	if err != nil {
		setupLog.Error(err, "unable to init ydblock")
		os.Exit(1)
	}
	if err = controllers.NewSaltFormulaReconciler(mgr, ylc).SetupWithManager(mgr); err != nil {
		setupLog.Error(err, "unable to create controller", "controller", controllers.ControllerName)
		os.Exit(1)
	}
	//+kubebuilder:scaffold:builder

	if err := mgr.AddHealthzCheck("healthz", healthz.Ping); err != nil {
		setupLog.Error(err, "unable to set up health check")
		os.Exit(1)
	}
	if err := mgr.AddReadyzCheck("readyz", healthz.Ping); err != nil {
		setupLog.Error(err, "unable to set up ready check")
		os.Exit(1)
	}

	setupLog.Info("starting manager")
	if err := mgr.Start(ctx); err != nil {
		setupLog.Error(err, "problem running manager")
		os.Exit(1)
	}
}
