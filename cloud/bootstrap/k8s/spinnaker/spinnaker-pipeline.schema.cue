package spinnaker

#Pipeline: ApplyAzPipeline | UpdateSaltFormulaVersionPipeline | OtherPipeline

GenericPipeline: {
	description?: string
	// The appConfig Schema
	appConfig: {
		...
	}

	// The application Schema
	application: string

	// The id Schema
	id: string

	// The index Schema
	index: int

	// The keepWaitingPipelines Schema
	keepWaitingPipelines: bool

	// The limitConcurrent Schema
	limitConcurrent: bool

	// The name Schema
	name: PipelineName

	// The parameterConfig Schema
	parameterConfig: [...(Parameter | ParameterWithOptions)]

	// The spelEvaluator Schema
	spelEvaluator: string

	// The stages Schema
	stages: [...{
		name:  string
		refId: string
		requisiteStageRefIds: [...string]
		type: string
		...
	}]

	// The triggers Schema
	triggers: [...]
}

OtherPipeline: GenericPipeline & {
	name: OtherPipelineName
}

PipelineName: ApplyAzPipelineName | UpdateSaltFormulaVersionPipelineName | OtherPipelineName

//To be sorted out
OtherPipelineName: "apply-az(HW&SVM)" | "apply-az(WIP)" | "apply-az(fixme)" | "Deploy cic-api" | "Deploy k8s manifest" |
	"Deploy manifests" | "Deploy vpc-api" |
	"Prepare artefacts" | "[WIP] Create ticket" | "[lb-duty-tools] deploy all" |
	"[lb-duty-tools] deploy manifest" | "bootstrap-salt-role" |
	"common-salt-role" | "deploy-compute-api" | "deploy-dummy" | "deploy-service-on-stand" |
	"get-current-gpu-version" | "get-gpu-version-from-salt-version" | "get-gpu-version-from-teamcity" |
	"is-greater-version" | "release" | "salt-runner-salt-role" | "solomon-agent-salt-role" |
	"test" | "update-helmfile-version-for-stand" |
	"update-salt-formula" | "update-salt-operator-version-for-stand(WIP)" |
	"update-startrek-ticket" |
	"[child] update salt version HW" | // BUG: Not work if added into UpdateSaltFormulaVersionPipelineName
	"update-teams-for-stand" | "update-teams" | "update-version-for-stand" |
	"update-version" | "update-yc-ci-verion-in-salt-formula" | "wait-salt-formula-commit" | "release-on-compute-node" |
	"[child]-update-helm-chart-version" |
	"common-resolve-slack-user-id" | "common-send-slack-message-to-author" | "db-migrate-on-stand" | "check-juggler" | "common-send-message-to-team"
