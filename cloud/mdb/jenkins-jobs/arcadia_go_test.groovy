job("arcadia-go-test"){
    description("""
<h2>Test time</h2>
<br/>
<img src='https://jenkins.db.yandex-team.ru/job/arcadia-go-test/buildTimeGraph/png' />""")
    logRotator(7, 30)
    label('built-in')
    concurrentBuild()

    authenticationToken('token')

    parameters {
        stringParam('ARCANUMID')
        stringParam('trigger_id')
    }

    wrappers {
        preBuildCleanup {
            deleteDirectories()
        }
        timestamps()
        colorizeOutput('xterm')
        credentialsBinding {
            string('ARCANUMAUTH', 'arcanum_token')
        }
        buildName('${ENV,var="ARCANUMID"} ${BUILD_NUMBER}')
        sshAgent('908ba07a-7d93-4cf1-9bb6-bce7533cb93c')
    }

    steps{
        shell('/usr/local/bin/arcadia_pass_context.py')
        systemGroovyCommand('''
import jenkins.*
import jenkins.model.*
import hudson.*
import hudson.model.*
import java.util.concurrent.CancellationException

def job_names = '%ARCADIA_DOWNSTREAM_PROJECTS%'

def build = Thread.currentThread().executable
def vars = build.getEnvVars()

properties_path = new File(build.workspace.toString() + "/infra.properties")
properties_is = new FileInputStream(properties_path)
properties = new Properties()
properties.load(properties_is)
properties_is.close()

ref_max_change = properties.MAX_SEEN_CHANGE.toInteger()

def builds = []
def futures = []

for (job_name in job_names.split(',')) {
  println("Triggering downstream job ${job_name}")
  def job = Hudson.instance.getJob(job_name)
  def runs = job._getRuns()
  def should_trigger = true
  def filters = []
  def found_filter = false

  if (job_name.contains("dataproc") && !job_name.contains("dataproc_image")) {
      println("No trigger for ${job_name} (irrelevant)")
      continue
  }
  if (job_name.contains("arcadia_infrastructure_kafka")) {
      if (!properties.SUMMARY.contains("[kafka]")) {
        println("No trigger for ${job_name} (irrelevant)")
        continue
      } else {
        found_filter = true
      }
  }
  if (properties.SUMMARY.contains("[noci]") && !job_name.contains("common")) {
      println("No trigger for ${job_name} (noci)")
      continue
  }
  for (filter in ["postgresql", "mongodb", "mysql", "redis", "clickhouse", "deploy_v2", "kafka", "sqlserver", "elasticsearch", "opensearch", "greenplum"]) {
      if (!properties.SUMMARY.contains("[" + filter + "]")) {
          continue
      }

      filters << filter
      if (job_name.contains(filter)) {
          found_filter = true
          break
      }
  }
  if (filters.size() > 0 && !found_filter) {
    println("No trigger for ${job_name} (has ${filters} filters but none matched this job)")
    should_trigger = false
  }

  for (run in runs) {
    environment = run.getEnvironment(null)
    result = run.getResult()
    review_id = environment.get('ARCANUMID')
    max_seen_change_str = environment.get('MAX_SEEN_CHANGE')
    if (max_seen_change_str == null || max_seen_change_str == "") {
      max_seen_change = 0
    } else {
      max_seen_change = max_seen_change_str.toInteger()
    }
    if (review_id != properties.ARCANUMID) {
      continue
    }
    if (max_seen_change < ref_max_change && (run.isBuilding() || run.hasntStartedYet())) {
      println("Terminating orphan ${run}")
      run.doStop()
    }
    if (max_seen_change == ref_max_change) {
      if (run.isBuilding() || run.hasntStartedYet()) {
        println("Adding ${run} to wait list")
        builds << run
        should_trigger = false
        break
      }
      if (result == Result.SUCCESS) {
        println("No trigger for ${job_name} (SUCCESS on ${run})")
        should_trigger = false
        break
      }
    }
  }
  if (should_trigger) {
    def params = [
      new StringParameterValue('ARCANUMID', properties.ARCANUMID),
      new StringParameterValue('MAX_SEEN_CHANGE', properties.MAX_SEEN_CHANGE),
      new StringParameterValue('SUMMARY', properties.SUMMARY),
    ]

    println("Triggering ${job_name} with ${properties.ARCANUMID}, ${properties.MAX_SEEN_CHANGE}")
    futures << job.scheduleBuild2(0, new Cause.UpstreamCause(build), new ParametersAction(params))
  }
}

println("Waiting for downstream jobs to complete")

should_wait = true
failed_jobs = []

deadline = System.currentTimeMillis() + 14400000L

while (should_wait) {
  done_futures = 0
  done_builds = 0
  for (future in futures) {
    if (future.isDone()) {
      job = future.get()
      if (!failed_jobs.contains(job) && job.result != Result.SUCCESS) {
        failed_jobs << job
      }
      done_futures += 1
    }
  }

  for (job in builds) {
    if (job.isBuilding() || job.hasntStartedYet()) {
      continue
    }
    if (!failed_jobs.contains(job) && job.getResult() != Result.SUCCESS) {
      failed_jobs << job
    }
    done_builds += 1
  }

  if (done_builds == builds.size() && done_futures == futures.size()) {
    should_wait = false
  } else {
    if (System.currentTimeMillis() > deadline) {
      throw new AbortException("Deadline reached")
    }
    sleep 1000
  }
}

if (failed_jobs.size() != 0) {
  for (job in failed_jobs) {
    println("Failed ${job.getAbsoluteUrl()}")
  }
  throw new AbortException("${failed_jobs.size()} tests failed")
}
''') {sandbox(true)}
    }

    publishers {
        wsCleanup {
            includePattern('**')
            deleteDirectories(true)
            setFailBuild(false)
        }
    }
}
