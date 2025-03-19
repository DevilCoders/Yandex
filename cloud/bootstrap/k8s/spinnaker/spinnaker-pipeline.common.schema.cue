package spinnaker

KnownNamespaces: "compute" | "bootstrap" | "gpu"
KnownBaseRoles:  "compute" | "bootstrap"
KnownSaltRoles:  "compute" | "bootstrap" | "compute-gpu" | "kubelet" | "solomon-agent"

ParameterGeneric: {
	name:        string
	label:       string
	description: string
	pinned:      bool
	required:    bool

	hasOptions: bool
	options: [... close({
		value: string
	})]
	default: string
}

//A parameter with no options
//Spinnaker may use single empty option to express no options
Parameter: ParameterGeneric & {
	hasOptions: false
	options:    [] | [ close({value: ""})]
}

ParameterWithOptions: ParameterGeneric & {
	hasOptions: true
	options: [...]
}

ParameterReleaseTicket: Parameter & {
	name:        "releaseTicket"
	label:       "Existing release ticket"
	description: "CLOUD-XXXXXX"
	pinned:      true
	required:    true
	default:     ""
}

ParameterSelectStand: ParameterWithOptions & {
	name:        "stand"
	label:       "Stand"
	description: ""
	pinned:      true
	required:    true

	hasOptions: true
	options: [
		{
			value: "testing"
		},
		{
			value: "preprod"
		},
		{
			value: "prod"
		},
	]
	default: ""
}

ParameterSelectAz: ParameterWithOptions & {
	name:        "az"
	label:       "AZ"
	description: ""
	pinned:      true
	required:    true

	hasOptions: true
	options: [
		{
			value: "a"
		},
		{
			value: "b"
		},
		{
			value: "c"
		},
	]
}

ParameterSaltFormulaVersion: Parameter & {
	description: "[Teamcity Job](https://teamcity.aw.cloud.yandex.net/buildConfiguration/Selfhost_YcSaltFormula_PackageBuild)"
	label:       "Salt formula version"
	name:        "saltFormulaVersion"
	pinned:      true
	required:    true
}

ApplyAzParameterSaltRole: Parameter & {
	name:        "saltRole"
	label:       "Salt role name"
	description: ""
	pinned:      true
	required:    true
}

ConfigureContextVar: {
	key:   string
	value: string | bool | int
}

ConfigureContextVarInSvmCluster: ConfigureContextVar & {
	key:   "inSvmCluster"
	value: bool
}

ConfigureContextVarNamespace: ConfigureContextVar & {
	key:   "namespace"
	value: KnownNamespaces
}
ConfigureContextVarBaseRole: ConfigureContextVar & {

	key:   "baseRole"
	value: KnownBaseRoles
}

ConfigureContextVarSaltRole: ConfigureContextVar & {

	key:   "saltRole"
	value: KnownSaltRoles
}

ConfigureContextVarReleaseTicket: ConfigureContextVar & {
	key:   "releaseTicket"
	value: "${parameters.releaseTicket.matches(\"CLOUD-[0-9]{5,6}\") == true ? parameters.releaseTicket: INVALID_TICKET_FORMAT}"
}

ConfigureContextVarStand: ConfigureContextVar & {
	key:   "stand"
	value: "${parameters.stand}"
}

ConfigureContextVarAz: ConfigureContextVar & {
	key:   "az"
	value: "${parameters.az}"
}

ConfigureContextVarExecutionURL: ConfigureContextVar & {
	key:   "spinnakerExecutionURL"
	value: "https://spinnaker.cloud.yandex.net/#/applications/${execution.application}/executions/${execution.Id}"
}

ConfigureContextVarExecutionMD: ConfigureContextVar & {
	key:   "spinnakerExecutionMD"
	value: "[${execution.Id}](${spinnakerExecutionURL})"
}

ConfigureContextVarTriggerUserLogin: ConfigureContextVar & {
	key:   "triggerUserLogin"
	value: "${trigger.user.replace(\"@yandex-team.ru\", \"\")}"
}

ConfigureContextVarSaltFormulaVersion: ConfigureContextVar & {
	key:   "saltFormulaVersion"
	value: "${parameters.saltFormulaVersion.matches(\"^0\\.1-[0-9]{4,6}\\.[0-9]{6}(\\.hotfix-[a-z_-]+)?$\") == true ? parameters.saltFormulaVersion: INVALID_SALT_FORMULA_VERSION_FORMAT}"
}

ConfigureContextVarOptionalClusterSuffix: ConfigureContextVar & {
	key:   "optionalClusterSuffix"
	value: "${inSvmCluster?\"-svm\":\"\"}"
}
BitbucketActionEditFile: {
	actionName:       "Update"
	actionType:       "editFile"
	bitbucketProfile: "bb.yandex-team.ru"
	branch:           string
	changes: [...BitbucketActionEditFileChange]
	commitMessage:    string
	filePath:         string
	ignoreUnmodified: false
	project:          "cloud"
	repo:             string
}

BitbucketActionEditFileChange: {
	changeType:  "regexReplace"
	expression:  string
	replacement: string
}

_ManifestFile: {
	_pipelineTraits: _PipelineTraits
	namespace:       "${namespace}"
	release:         "${releaseName}" | "${baseRole}"
	chart:           "${chartName}" | "salt-base-role"
	fileName:        "${manifestFileName}" | "${saltRole}.yaml"
	filePathInRepo:  "${stand}/ru-central1-${az}\(_pipelineTraits.optionalClusterSuffix)/manifests/\(namespace)/\(release)/\(chart)/templates/\(fileName)"
}

ManifestFileContainter: _ManifestFile & {
	_pipelineTraits: isSaltRole: false
	release:  "${releaseName}"
	chart:    "${chartName}"
	fileName: "${manifestFileName}"
}

ManifestFileSalt: _ManifestFile & {
	_pipelineTraits: isSaltRole: true
	release:  "${baseRole}"
	chart:    "salt-base-role"
	fileName: "${saltRole}.yaml"
}

ManifestFile: ManifestFileContainter | ManifestFileSalt
