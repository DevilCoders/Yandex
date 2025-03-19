package server

import (
	"context"
	"sort"
	"time"

	pb "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/dataproc/manager/v1"
	intapi "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/dataproc/v1"
	ig "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/microcosm/instancegroup/v1"
	datastoreBackend "a.yandex-team.ru/cloud/mdb/dataproc-manager/pkg/datastore"
	"a.yandex-team.ru/cloud/mdb/dataproc-manager/pkg/models"
	"a.yandex-team.ru/cloud/mdb/dataproc-manager/pkg/models/health"
	"a.yandex-team.ru/cloud/mdb/dataproc-manager/pkg/models/service"
	"a.yandex-team.ru/cloud/mdb/internal/semerr"
	"a.yandex-team.ru/library/go/core/xerrors"
)

// pass revision=-1 in order to return ClusterTopology of any revision
func (s *Server) getClusterTopology(ctx context.Context, cid string, revision int64) (models.ClusterTopology, error) {
	topology, err := s.datastore.GetCachedClusterTopology(ctx, cid)
	if err != nil && err != datastoreBackend.ErrNotFound {
		return models.ClusterTopology{}, xerrors.Errorf("failed to get topology from datastore: %w", err)
	}
	if err == nil {
		if revision == -1 || topology.Revision == revision {
			return topology, nil
		} else if topology.Revision > revision {
			s.logger.Debugf("Report with old topology revision: %+v", topology)
			return models.ClusterTopology{}, xerrors.New("report with old topology revision")
		}
	}

	topology, err = s.intAPI.GetClusterTopology(ctx, cid)
	if err != nil {
		return models.ClusterTopology{}, xerrors.Errorf("failed to get topology from internal API: %w", err)
	}
	err = s.datastore.StoreClusterTopology(ctx, cid, topology)
	if err != nil {
		s.logger.Errorf("Failed to save topology to datastore: %s", err)
	}
	return topology, nil
}

// get only hosts with Alive status
func getHostsReadyForDecommission(decommissionHosts models.DecommissionHosts, hostsHealth map[string]models.HostHealth) ([]string, []string) {
	var yarnRunningHosts []string
	var hdfsRunningHosts []string
	for _, host := range decommissionHosts.YarnHosts {
		if hostHealth, exists := hostsHealth[host]; exists {
			if _, exists := hostHealth.Services[service.Yarn]; exists {
				yarnHealth := hostHealth.ServiceHealth(service.Yarn)
				if yarnHealth == health.Alive || yarnHealth == health.Decommissioning {
					yarnRunningHosts = append(yarnRunningHosts, host)
				}
			}
		}
	}
	for _, host := range decommissionHosts.HdfsHosts {
		if hostHealth, exists := hostsHealth[host]; exists {
			if _, exists := hostHealth.Services[service.Hdfs]; exists {
				hdfsHealth := hostHealth.ServiceHealth(service.Hdfs)
				if hdfsHealth == health.Alive || hdfsHealth == health.Decommissioning {
					hdfsRunningHosts = append(hdfsRunningHosts, host)
				}
			}
		}
	}
	return yarnRunningHosts, hdfsRunningHosts
}

func getMapKeys(dictionary map[string]string) []string {
	keys := make([]string, 0, len(dictionary))
	for key := range dictionary {
		keys = append(keys, key)
	}
	return keys
}

func (s *Server) decommissionHosts(ctx context.Context, managedInstancesToDecommission map[string]string,
	topology models.ClusterTopology, hostsHealth map[string]models.HostHealth, decommissionTimeout int64,
	instanceGroupID string) error {
	if decommissionTimeout > 0 {
		s.logger.Infof(
			"starting to decommission with timeout=%d. hosts: %s",
			decommissionTimeout,
			managedInstancesToDecommission,
		)
		err := s.datastore.StoreDecommissionHosts(
			ctx,
			topology.Cid,
			models.DecommissionHosts{
				YarnHosts: getMapKeys(managedInstancesToDecommission),
				Timeout:   decommissionTimeout,
			},
		)
		if err != nil {
			return xerrors.Errorf("error while saving to datastore: %w", err)
		}
	}
	var managedInstanceIDsToDelete []string
	for fqdn, managedInstanceID := range managedInstancesToDecommission {
		hostHealth, ok := hostsHealth[fqdn]
		isDecommissioned := ok && hostHealth.Services[service.Yarn].GetHealth() == health.Decommissioned
		// Instance Group Service ManagedInstanceId != compute service instanceId.
		// It is an Instance Group Service entity and these ids should be passed to DeleteInstances method
		if decommissionTimeout == 0 || isDecommissioned || !ok {
			s.logger.Infof(
				"Trying to delete decommissioned instance %s fqdn %s with timeout %d",
				managedInstanceID,
				fqdn,
				decommissionTimeout,
			)
			managedInstanceIDsToDelete = append(managedInstanceIDsToDelete, managedInstanceID)
		}
	}
	if len(managedInstanceIDsToDelete) > 0 {
		_, err := s.instanceGroupServiceClient.StopInstances(
			ctx,
			instanceGroupID,
			managedInstanceIDsToDelete,
			topology.ServiceAccountID,
			topology.GetSubclusterByInstanceGroupID(instanceGroupID).Subcid,
		)
		if err != nil {
			return xerrors.Errorf(
				"error while starting to delete managed instances %s: %w",
				managedInstanceIDsToDelete,
				err,
			)
		}
	}

	return nil
}

func (s *Server) getManagedInstancesToDelete(
	instances []*ig.ManagedInstance,
	numberOfHostsToDecommission int64,
) map[string]string {
	managedInstancesToDecommission := make(map[string]string)
	for _, instance := range instances {
		if s.instanceGroupServiceClient.IsPreferredForDecommission(instance) {
			fqdn := s.instanceGroupServiceClient.GetUserSpecifiedFQDN(instance)
			managedInstancesToDecommission[fqdn] = instance.GetId()
			if len(managedInstancesToDecommission) >= int(numberOfHostsToDecommission) {
				break
			}
		}
	}
	// sort by modification time descending
	sort.Slice(instances, func(i, j int) bool {
		return instances[i].GetStatusChangedAt().GetSeconds() > instances[j].GetStatusChangedAt().GetSeconds()
	})
	for _, instance := range instances {
		if s.instanceGroupServiceClient.IsRunning(instance) {
			fqdn := s.instanceGroupServiceClient.GetUserSpecifiedFQDN(instance)
			managedInstancesToDecommission[fqdn] = instance.GetId()
			if len(managedInstancesToDecommission) >= int(numberOfHostsToDecommission) {
				break
			}
		}
	}
	return managedInstancesToDecommission
}

func (s *Server) checkInstanceGroup(
	ctx context.Context,
	instanceGroupID string,
	topology models.ClusterTopology,
	hostsHealth map[string]models.HostHealth) {
	instanceGroup, err := s.instanceGroupServiceClient.Get(ctx, instanceGroupID, topology.ServiceAccountID)
	if err != nil {
		s.logger.Errorf("could not get instance group %s error: %+v", instanceGroupID, err)
		return
	}
	managedState := instanceGroup.GetManagedInstancesState()
	numberOfHostsToDecommission := managedState.GetRunningActualCount() - managedState.GetTargetSize()
	if numberOfHostsToDecommission < 0 {
		numberOfHostsToDecommission = 0
	}
	numberOfHostsToDecommission += managedState.GetRunningOutdatedCount()
	if numberOfHostsToDecommission > 0 {
		s.logger.Infof(
			"found %d hosts to be decommissioned in instance group %s",
			numberOfHostsToDecommission,
			instanceGroupID,
		)
		instances, err := s.instanceGroupServiceClient.ListInstances(ctx, instanceGroupID, topology.ServiceAccountID)
		if instances == nil {
			s.logger.Errorf("could not list instances of instance group %s error: %+v", instanceGroupID, err)
			return
		}
		managedInstancesToDecommission := s.getManagedInstancesToDelete(instances, numberOfHostsToDecommission)
		subclusterTopology := topology.GetSubclusterByInstanceGroupID(instanceGroupID)
		if subclusterTopology != nil {
			decommissionTimeout := subclusterTopology.DecommissionTimeout
			err = s.decommissionHosts(
				ctx,
				managedInstancesToDecommission,
				topology,
				hostsHealth,
				decommissionTimeout,
				instanceGroupID,
			)
			if err != nil {
				s.logger.Errorf("instance group decommission error: %+v", err)
			}
		} else {
			s.logger.Errorf("could not get subcluster topology by instance group id: %+v", instanceGroupID)
		}
	}
}

// Report handles Report request
func (s *Server) Report(ctx context.Context, report *pb.ReportRequest) (*pb.ReportReply, error) {
	cid := report.Cid

	s.logger.Debugf("Cid %v. Got report: %+v", cid, report)

	err := s.authorizeClusterAccess(ctx, cid)
	if err != nil {
		s.logger.Debugf("Cid %v. Authorization error: %+v", cid, err)
		return nil, err
	}

	s.reportCount = report.Info.ReportCount

	topology, err := s.getClusterTopology(ctx, cid, report.TopologyRevision)
	if err != nil {
		return nil, xerrors.Errorf("failed to get cluster topology: %w", err)
	}
	s.logger.Debugf("Cid %v. Got topology: %+v", cid, topology)

	clusterHealth, hostsHealth := deduceClusterHealth(report, topology, time.Now().Unix())
	clusterHealth.Cid = cid

	s.logger.Debugf("Cid %v. Deduced cluster health: %+v", cid, clusterHealth)
	s.logger.Debugf("Cid %v. Deduced hosts health: %+v", cid, hostsHealth)

	err = s.datastore.StoreClusterHealth(ctx, cid, clusterHealth)
	if err != nil {
		return nil, xerrors.Errorf("error while saving to datastore: %w", err)
	}

	err = s.datastore.StoreHostsHealth(ctx, cid, hostsHealth)
	if err != nil {
		return nil, xerrors.Errorf("error while saving to datastore: %w", err)
	}
	s.logger.Debugf("Received ReportRequest. Cluster %+v, health %+s", clusterHealth.Cid, clusterHealth.Health)

	instanceGroupIDs := topology.GetInstanceGroupIDs()
	if len(instanceGroupIDs) > 0 {
		for _, instanceGroupID := range instanceGroupIDs {
			s.checkInstanceGroup(
				ctx,
				instanceGroupID,
				topology,
				hostsHealth)
		}
	}

	decommissionHosts, err := s.datastore.LoadDecommissionHosts(
		ctx,
		cid,
	)
	if err != nil {
		return nil, xerrors.Errorf("error loading decommission hosts from datastore: %w", err)
	}
	yarnRunningHosts, hdfsRunningHosts := getHostsReadyForDecommission(decommissionHosts, hostsHealth)
	decommissionHosts.YarnHosts = yarnRunningHosts
	decommissionHosts.HdfsHosts = hdfsRunningHosts
	if decommissionHosts.YarnHosts != nil || decommissionHosts.HdfsHosts != nil {
		err = s.datastore.StoreDecommissionHosts(
			ctx,
			cid,
			decommissionHosts,
		)
	} else {
		err = s.datastore.DeleteDecommissionHosts(
			ctx,
			cid,
		)
	}
	if err != nil {
		return nil, xerrors.Errorf("error during working with decommission datastore: %w", err)
	}

	err = s.datastore.StoreDecommissionStatus(
		ctx,
		cid,
		models.DecommissionStatus{
			YarnRequestedDecommissionHosts: report.Info.Yarn.RequestedDecommissionHosts,
			HdfsRequestedDecommissionHosts: report.Info.Hdfs.RequestedDecommissionHosts,
		},
	)
	if err != nil {
		return nil, xerrors.Errorf("error while saving decommission status to datastore: %w", err)
	}

	return &pb.ReportReply{
		DecommissionTimeout:     decommissionHosts.Timeout,
		YarnHostsToDecommission: yarnRunningHosts,
		HdfsHostsToDecommission: hdfsRunningHosts,
	}, nil
}

// Handles Decommission request
func (s *Server) Decommission(ctx context.Context, request *pb.DecommissionRequest) (*pb.DecommissionReply, error) {
	cid := request.Cid

	s.logger.Debugf("Decommission Cid %v. Got request: %+v", cid, request)

	err := s.datastore.StoreDecommissionHosts(
		ctx,
		cid,
		models.DecommissionHosts{YarnHosts: request.YarnHosts, HdfsHosts: request.HdfsHosts, Timeout: request.Timeout},
	)
	if err != nil {
		return nil, xerrors.Errorf("error while saving to datastore: %w", err)
	}

	return &pb.DecommissionReply{}, nil
}

// Handles DecommissionStatus request
func (s *Server) DecommissionStatus(ctx context.Context, request *pb.DecommissionStatusRequest) (*pb.DecommissionStatusReply, error) {
	cid := request.Cid

	s.logger.Debugf("Decommission Status Cid %v. Got request: %+v", cid, request)

	status, err := s.datastore.LoadDecommissionStatus(
		ctx,
		cid,
	)
	if err != nil {
		return nil, xerrors.Errorf("error while saving to datastore: %w", err)
	}

	return &pb.DecommissionStatusReply{
		YarnRequestedDecommissionHosts: status.YarnRequestedDecommissionHosts,
		HdfsRequestedDecommissionHosts: status.HdfsRequestedDecommissionHosts,
	}, nil
}

func (s *Server) healthMarshal(healthStatus health.Health) pb.Health {
	healthStatusMap := map[health.Health]pb.Health{
		health.Unknown:         pb.Health_HEALTH_UNSPECIFIED,
		health.Disabled:        pb.Health_HEALTH_UNSPECIFIED,
		health.Dead:            pb.Health_DEAD,
		health.Degraded:        pb.Health_DEGRADED,
		health.Alive:           pb.Health_ALIVE,
		health.Decommissioning: pb.Health_DECOMMISSIONING,
		health.Decommissioned:  pb.Health_DECOMMISSIONED,
	}
	healthStatusResult, ok := healthStatusMap[healthStatus]
	if !ok {
		s.logger.Errorf("Unknown mapping for health status %+v", healthStatus)
		return pb.Health_HEALTH_UNSPECIFIED
	}
	return healthStatusResult
}

func (s *Server) serviceMarshal(serviceType service.Service) pb.Service {
	serviceTypeMap := map[service.Service]pb.Service{
		service.Hdfs:      pb.Service_HDFS,
		service.Yarn:      pb.Service_YARN,
		service.Mapreduce: pb.Service_MAPREDUCE,
		service.Hive:      pb.Service_HIVE,
		service.Tez:       pb.Service_TEZ,
		service.Zookeeper: pb.Service_ZOOKEEPER,
		service.Hbase:     pb.Service_HBASE,
		service.Sqoop:     pb.Service_SQOOP,
		service.Flume:     pb.Service_FLUME,
		service.Spark:     pb.Service_SPARK,
		service.Oozie:     pb.Service_OOZIE,
		service.Livy:      pb.Service_LIVY,
	}
	serviceTypeResult, ok := serviceTypeMap[serviceType]
	if !ok {
		s.logger.Errorf("Unknown mapping for service type %+v", serviceType)
		return pb.Service_SERVICE_UNSPECIFIED
	}
	return serviceTypeResult
}

// ClusterHealth handles ClusterHealth request
func (s *Server) ClusterHealth(ctx context.Context, in *pb.ClusterHealthRequest) (*pb.ClusterHealthReply, error) {
	clusterHealth, err := s.datastore.LoadClusterHealth(ctx, in.Cid)
	if err == datastoreBackend.ErrNotFound {
		s.logger.Debugf("Failed to get cluster health: %+v, Cid %s not found", err, in.Cid)
		return nil, semerr.NotFoundf("Cid '%s' not found", in.Cid)
	}
	if err != nil {
		return nil, xerrors.Errorf("failed to get cluster health: %w", err)
	}

	serviceHealthResult := make([]*pb.ServiceHealth, 0, len(clusterHealth.Services))
	for serviceType, serviceHealth := range clusterHealth.Services {
		serviceHealthResult = append(serviceHealthResult, &pb.ServiceHealth{
			Service: s.serviceMarshal(serviceType),
			Health:  s.healthMarshal(serviceHealth.GetHealth()),
		})
	}

	s.logger.Debugf("Received ClusterHealthRequest. Cluster %+v, health %+s", clusterHealth.Cid, clusterHealth.Health)
	return &pb.ClusterHealthReply{
		Cid:            in.Cid,
		Health:         s.healthMarshal(clusterHealth.Health),
		ServiceHealth:  serviceHealthResult,
		Explanation:    clusterHealth.Explanation,
		HdfsInSafemode: clusterHealth.HdfsInSafemode,
		ReportCount:    s.reportCount,
		UpdateTime:     clusterHealth.UpdateTime,
	}, nil
}

// HostsHealth handles HostsHealth request
func (s *Server) HostsHealth(ctx context.Context, in *pb.HostsHealthRequest) (*pb.HostsHealthReply, error) {
	hostsHealth, err := s.datastore.LoadHostsHealth(ctx, in.Cid, in.Fqdns)
	if err != nil {
		return nil, xerrors.Errorf("failed to get hosts health: %w", err)
	}
	hostsHealthResponse := make([]*pb.HostHealth, 0, len(hostsHealth))
	for fqdn, hostHealth := range hostsHealth {
		serviceHealthResult := make([]*pb.ServiceHealth, 0, len(hostHealth.Services))
		for serviceType, serviceHealth := range hostHealth.Services {
			serviceHealthResult = append(serviceHealthResult, &pb.ServiceHealth{
				Service: s.serviceMarshal(serviceType),
				Health:  s.healthMarshal(serviceHealth.GetHealth()),
			})
		}
		hostsHealthResponse = append(hostsHealthResponse, &pb.HostHealth{
			Fqdn:          fqdn,
			Health:        s.healthMarshal(hostHealth.Health),
			ServiceHealth: serviceHealthResult,
		})
	}

	s.logger.Debugf("Received HostHealthRequest. Cluster %+v, health: %+v", in.Cid, hostsHealthResponse)
	return &pb.HostsHealthReply{
		HostsHealth: hostsHealthResponse,
	}, nil
}

func (s *Server) ListActive(ctx context.Context, in *pb.ListJobsRequest) (*pb.ListJobsResponse, error) {
	err := s.authorizeClusterAccess(ctx, in.ClusterId)
	if err != nil {
		return nil, err
	}

	req := &intapi.ListJobsRequest{
		ClusterId: in.ClusterId,
		PageSize:  in.PageSize,
		PageToken: in.PageToken,
		Filter:    "status='active'",
	}
	intapiResponse, err := s.intAPI.ListClusterJobs(ctx, req)
	if err != nil {
		return nil, xerrors.Errorf("failed to get a list of jobs from internal api for cluster %s: %w", in.ClusterId, err)
	}

	dmJobs := make([]*pb.Job, 0, len(intapiResponse.Jobs))
	for _, intapiJob := range intapiResponse.Jobs {
		dmJob, err := convertJob(intapiJob)
		if err != nil {
			return nil, xerrors.Errorf("failed to convert job id=%s from intapi to dm format: %w", intapiJob.Id, err)
		} else {
			dmJobs = append(dmJobs, dmJob)
		}
	}

	managerResponse := &pb.ListJobsResponse{
		Jobs:          dmJobs,
		NextPageToken: intapiResponse.NextPageToken,
	}
	return managerResponse, nil
}

func (s *Server) UpdateStatus(ctx context.Context, in *pb.UpdateJobStatusRequest) (*pb.UpdateJobStatusResponse, error) {
	err := s.authorizeClusterAccess(ctx, in.ClusterId)
	if err != nil {
		return nil, err
	}

	statusName := in.Status.String()
	statusID, found := intapi.Job_Status_value[statusName]
	if !found {
		return nil, xerrors.Errorf("failed to convert status %s of job id=%s from dm to intapi format",
			statusName, in.JobId)
	}

	req := &intapi.UpdateJobStatusRequest{
		ClusterId: in.ClusterId,
		JobId:     in.JobId,
		Status:    intapi.Job_Status(statusID),
	}
	if in.ApplicationInfo != nil {
		req.ApplicationInfo = &intapi.ApplicationInfo{
			Id: in.ApplicationInfo.Id,
		}
		for _, applicationAttempt := range in.ApplicationInfo.ApplicationAttempts {
			req.ApplicationInfo.ApplicationAttempts = append(req.ApplicationInfo.ApplicationAttempts,
				&intapi.ApplicationAttempt{
					Id:            applicationAttempt.Id,
					AmContainerId: applicationAttempt.AmContainerId,
				},
			)
		}
	}
	_, err = s.intAPI.UpdateJobStatus(ctx, req)
	if err != nil {
		return nil, xerrors.Errorf("failed to update state of job %s: %w", in.JobId, err)
	}

	managerResponse := &pb.UpdateJobStatusResponse{}
	return managerResponse, nil
}

func convertJob(intapiJob *intapi.Job) (*pb.Job, error) {
	statusName := intapiJob.Status.String()
	statusID, found := pb.Job_Status_value[statusName]
	if !found {
		return nil, xerrors.Errorf("failed to convert job status %v", statusName)
	}

	dmJob := pb.Job{
		Id:         intapiJob.Id,
		ClusterId:  intapiJob.ClusterId,
		CreatedAt:  intapiJob.CreatedAt,
		StartedAt:  intapiJob.StartedAt,
		FinishedAt: intapiJob.FinishedAt,
		Name:       intapiJob.Name,
		Status:     pb.Job_Status(statusID),
	}

	switch intapiJobSpec := intapiJob.JobSpec.(type) {
	case *intapi.Job_MapreduceJob:
		intapiMapreduceJob := intapiJobSpec.MapreduceJob
		dmMapreduceJob := pb.MapreduceJob{
			Args:        intapiMapreduceJob.Args,
			JarFileUris: intapiMapreduceJob.JarFileUris,
			FileUris:    intapiMapreduceJob.FileUris,
			ArchiveUris: intapiMapreduceJob.ArchiveUris,
			Properties:  intapiMapreduceJob.Properties,
		}

		switch intapiDriver := intapiMapreduceJob.Driver.(type) {
		case *intapi.MapreduceJob_MainJarFileUri:
			dmMapreduceJob.Driver = &pb.MapreduceJob_MainJarFileUri{MainJarFileUri: intapiDriver.MainJarFileUri}
		case *intapi.MapreduceJob_MainClass:
			dmMapreduceJob.Driver = &pb.MapreduceJob_MainClass{MainClass: intapiDriver.MainClass}
		default:
			return nil, xerrors.Errorf("failed to convert mapreduce job driver %+v", intapiMapreduceJob.Driver)
		}

		dmJob.JobSpec = &pb.Job_MapreduceJob{MapreduceJob: &dmMapreduceJob}
	case *intapi.Job_SparkJob:
		intapiSparkJob := intapiJobSpec.SparkJob
		dmSparkJob := pb.SparkJob{
			Args:            intapiSparkJob.Args,
			JarFileUris:     intapiSparkJob.JarFileUris,
			FileUris:        intapiSparkJob.FileUris,
			ArchiveUris:     intapiSparkJob.ArchiveUris,
			Properties:      intapiSparkJob.Properties,
			MainJarFileUri:  intapiSparkJob.MainJarFileUri,
			MainClass:       intapiSparkJob.MainClass,
			Packages:        intapiSparkJob.Packages,
			Repositories:    intapiSparkJob.Repositories,
			ExcludePackages: intapiSparkJob.ExcludePackages,
		}
		dmJob.JobSpec = &pb.Job_SparkJob{SparkJob: &dmSparkJob}
	case *intapi.Job_PysparkJob:
		intapiPysparkJob := intapiJobSpec.PysparkJob
		dmPysparkJob := pb.PysparkJob{
			Args:              intapiPysparkJob.Args,
			JarFileUris:       intapiPysparkJob.JarFileUris,
			FileUris:          intapiPysparkJob.FileUris,
			ArchiveUris:       intapiPysparkJob.ArchiveUris,
			Properties:        intapiPysparkJob.Properties,
			MainPythonFileUri: intapiPysparkJob.MainPythonFileUri,
			PythonFileUris:    intapiPysparkJob.PythonFileUris,
			Packages:          intapiPysparkJob.Packages,
			Repositories:      intapiPysparkJob.Repositories,
			ExcludePackages:   intapiPysparkJob.ExcludePackages,
		}
		dmJob.JobSpec = &pb.Job_PysparkJob{PysparkJob: &dmPysparkJob}
	case *intapi.Job_HiveJob:
		intapiHiveJob := intapiJobSpec.HiveJob
		dmHiveJob := pb.HiveJob{
			Properties:        intapiHiveJob.Properties,
			ContinueOnFailure: intapiHiveJob.ContinueOnFailure,
			ScriptVariables:   intapiHiveJob.ScriptVariables,
			JarFileUris:       intapiHiveJob.JarFileUris,
		}

		switch intapiQueryType := intapiHiveJob.QueryType.(type) {
		case *intapi.HiveJob_QueryFileUri:
			dmHiveJob.QueryType = &pb.HiveJob_QueryFileUri{QueryFileUri: intapiQueryType.QueryFileUri}
		case *intapi.HiveJob_QueryList:
			dmHiveJob.QueryType = &pb.HiveJob_QueryList{
				QueryList: &pb.QueryList{Queries: intapiQueryType.QueryList.Queries},
			}
		default:
			return nil, xerrors.Errorf("failed to convert hive job query type %+v", intapiHiveJob.QueryType)
		}

		dmJob.JobSpec = &pb.Job_HiveJob{HiveJob: &dmHiveJob}
	default:
		return nil, xerrors.Errorf("failed to convert job spec %+v", intapiJob.JobSpec)
	}

	return &dmJob, nil
}
