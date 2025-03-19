package spinnaker

ApplyAzPipelineName: "[child] apply az" | "[child] apply az HW" | "[child] apply az SVM"

ApplyAzPipeline: ApplyAzPipelineAnyContainerHW | ApplyAzPipelineAnyContainerSVM | ApplyAzPipelineSaltRoleHW | ApplyAzPipelineSaltRoleSVM

ApplyAzPipelineGeneric: GenericPipeline & {
	_traits: _PipelineTraits
	name:    ApplyAzPipelineName
	parameterConfig: [ParameterReleaseTicket, ParameterSelectStand, ParameterSelectAz]
	stages: [
		ApplyAzStageConfigureContext & {_pipelineTraits: _traits},
		ApplyAzReadManifest,
		ApplyAzShowManifest,
		ApplyAzStageCreateInfraEvent,
		ApplyAzStageDeployManifest,
		ApplyAzStageCloseInfraEvent,
		ApplyAzStageCheckSuccess,
	]
}

ApplyAzPipelineAnyContainerHW: ApplyAzPipelineGeneric & {
	_traits: PipelineTraitsAnyContainerHW
	name:    "[child] apply az" | "[child] apply az HW"
}

ApplyAzPipelineAnyContainerSVM: ApplyAzPipelineGeneric & {
	_traits: PipelineTraitsAnyContainerSVM
	name:    "[child] apply az" | "[child] apply az SVM"
}

ApplyAzPipelineSaltRoleHW: ApplyAzPipelineGeneric & {
	_traits: PipelineTraitsSaltRoleHW
	name:    "[child] apply az" | "[child] apply az HW"
}

ApplyAzPipelineSaltRoleSVM: ApplyAzPipelineGeneric & {
	_traits: PipelineTraitsSaltRoleSVM
	name:    "[child] apply az" | "[child] apply az SVM"
}

ApplyAzStageConfigureContext: {
	_pipelineTraits: _PipelineTraits
	let traits = _pipelineTraits
	_manifestFile:           ManifestFile & {_pipelineTraits: traits}
	failOnFailedExpressions: true
	name:                    "Configure context"
	refId:                   "10"
	requisiteStageRefIds: []
	type: "evaluateVariables"
	variables: [...ConfigureContextVar]
	variables: [
		//Application specific variables
		ConfigureContextVarNamespace,
		if _pipelineTraits.isSaltRole {
			ConfigureContextVarBaseRole
		},
		if _pipelineTraits.isSaltRole {
			ConfigureContextVarSaltRole
		},
		if !_pipelineTraits.isSaltRole {
			{
				key:   "releaseName"
				value: string
			}
		},
		if !_pipelineTraits.isSaltRole {
			{
				key:   "chartName"
				value: string
			}
		},
		if !_pipelineTraits.isSaltRole {
			{
				key:   "manifestFileName"
				value: =~"^[^/]+.yaml$"
			}
		},
		{
			key:   "infraServiceId"
			value: int
		},
		{
			key:   "infraServiceTestingEnvId"
			value: int
		},
		{
			key:   "infraServicePreprodEnvId"
			value: int
		},
		{
			key:   "infraServiceProdEnvId"
			value: int
		},

		//Evaluate vars
		ConfigureContextVarExecutionURL,
		ConfigureContextVarTriggerUserLogin,
		{//TODO move me to InfraEvent step
			key:   "infraEnvironmentID"
			value: "${parameters.stand == \"testing\" ? infraServiceTestingEnvId : parameters.stand == \"preprod\" ? infraServicePreprodEnvId : parameters.stand == \"prod\" ? infraServiceProdEnvId : FAIL }"
		},
		ConfigureContextVarStand,
		ConfigureContextVarAz,
		{//TODO move me to InfraEvent step
			key:   "dc"
			value: "${parameters.az == \"a\" ? \"vla\" : parameters.az == \"b\" ? \"sas\" : parameters.az == \"c\" ? \"myt\" : FAIL }"
		},
		ConfigureContextVarReleaseTicket,
		{
			key:   "accountName"
			value: "kten_${stand}-${az}\(_pipelineTraits.optionalClusterSuffix)_${namespace}"
		},
		{
			key:   "pathToManifest"
			value: _manifestFile.filePathInRepo
		},
	]
}

ApplyAzReadManifest: {
	actions: [
		{
			actionId:         "b27dc902-2658-4389-9dc1-717ef2d61202"
			actionName:       "Read manifest"
			actionType:       "readFile"
			bitbucketProfile: "bb.yandex-team.ru"
			branch:           "master"
			filePath:         "${pathToManifest}"
			outputVariable:   "manifestToApply"
			project:          "cloud"
			repo:             "k8s-deploy"
		},
	]
	expectedArtifacts: [
		{
			defaultArtifact: {
				customKind: true
				id:         "e542f9b0-a23a-4ffa-8b2e-b85e4f618dee"
			}
			displayName: "manifestToApply"
			id:          "ac94817c-f8e6-49d8-8153-96ec513c5ae7"
			matchArtifact: {
				"artifactAccount": "embedded-artifact"
				id:                "4efa8549-0dee-475c-881d-620ae820085d"
				name:              "${manifestToApply}"
				type:              "embedded/base64"
			}
			useDefaultArtifact: false
			usePriorArtifact:   false
		},
	]
	name:  "Read manifest"
	refId: "20"
	requisiteStageRefIds: [
		"10",
	]
	type: "bitbucket"
}
ApplyAzShowManifest: {
	failOnFailedExpressions: true
	name:                    "Show manifest"
	refId:                   "30"
	requisiteStageRefIds: [
		"20",
	]
	type: "evaluateVariables"
	variables: [
		{
			key:   "manifestToApply"
			value: "${#readYaml(manifestToApply)}"
		},
	]
}

ApplyAzStageCreateInfraEvent: {
	comments?:     string
	description:   "Started by: ${triggerUserLogin} Execution: ${spinnakerExecutionURL}"
	environmentId: "${infraEnvironmentID}"
	eventType:     "maintenance"
	myt:           "${dc == \"myt\"}"
	name:          "Create Infra event"
	refId:         "40"
	requisiteStageRefIds: [
		"30",
	]
	sas:       "${dc == \"sas\"}"
	serviceId: "${infraServiceId}"
	severity:  "major"
	tickets:   "${releaseTicket}"
	title:     "Release"
	type:      "createInfraEvent"
	vla:       "${dc == \"vla\"}"
}

ApplyAzStageDeployManifest: {
	account:                       "${accountName}"
	cloudProvider:                 "kubernetes"
	completeOtherBranchesThenFail: false
	continuePipeline:              true
	failPipeline:                  false
	manifestArtifact: {
		artifactAccount: "bb.yandex-team.ru"
		id:              "121a759b-7739-4c70-acfb-1af06a17c9d7"
		name:            "${pathToManifest}"
		reference:       "https://bb.yandex-team.ru/projects/CLOUD/repos/k8s-deploy/raw/${pathToManifest}"
		type:            "bitbucket/file"
	}
	moniker: {
		app: "${execution.application}"
	}
	name:              "Deploy k8s"
	namespaceOverride: ""
	refId:             "50"
	requisiteStageRefIds: [
		"40",
	]
	skipExpressionEvaluation: true
	source:                   "artifact"
	trafficManagement: {
		enabled: false
		options: {
			enableTraffic: false
		}
	}
	type: "deployManifest"
}

ApplyAzStageCloseInfraEvent: {
	name:  "Close Infra event"
	refId: "60"
	requisiteStageRefIds: [
		"50",
	]
	type: "closeInfraEvent"
}

ApplyAzStageCheckSuccess: {
	name: "Check state"
	preconditions: [
		{
			context: {
				stageName:   "Deploy k8s"
				stageStatus: "SUCCEEDED"
			}
			failPipeline: true
			type:         "stageStatus"
		},
	]
	refId: "70"
	requisiteStageRefIds: [
		"60",
	]
	type: "checkPreconditions"
}
