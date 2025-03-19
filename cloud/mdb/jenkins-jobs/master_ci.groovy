job("master-ci"){
    description("""
<h2>Test time</h2>
<br/>
<img src='https://jenkins.db.yandex-team.ru/job/master-ci/buildTimeGraph/png' />""")
    logRotator(30, 200, 30, 200)
    concurrentBuild()
    label('built-in')

    triggers {
        cron('H */6 * * *')
    }

    wrappers {
        timestamps()
        buildName('master-ci ${BUILD_NUMBER}')
        colorizeOutput('xterm')
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
passed_jobs = []

deadline = System.currentTimeMillis() + 21600000L

while (should_wait) {
  done_futures = 0
  for (future in futures) {
    if (future.isDone()) {
      job = future.get()
      if (!failed_jobs.contains(job) && job.result != Result.SUCCESS) {
        failed_jobs << job
      }
      if (!passed_jobs.contains(job) && job.result == Result.SUCCESS) {
        passed_jobs << job
      }
      done_futures += 1
    }
  }
  if (done_futures == futures.size()) {
    should_wait = false
  } else {
    if (System.currentTimeMillis() > deadline) {
      throw new AbortException("Deadline reached")
    }
    sleep 1000
  }
}

for (job in failed_jobs) {
  println("Failed ${job.getAbsoluteUrl()}")
}

for (job in passed_jobs) {
  println("Passed ${job.getAbsoluteUrl()}")
}

for (future in futures) {
  result = future.get()
  manager = result.getArtifactManager()
  for (artifact in result.getArtifacts()) {
    path = artifact.relativePath
    if (!path.contains('/junit/') || !path.contains('.xml')) {
      continue
    }
    artifact_file = manager.root().child(path)
    stream = artifact_file.open()
    out_file = new File(build.workspace.toString() + '/' + path.split('/')[-1])
    out_path = new FilePath(out_file)
    out_path.copyFrom(stream)
    stream.close()
  }
}

if (failed_jobs.size() != 0) {
  throw new AbortException("${failed_jobs.size()} tests failed")
}
''') {sandbox(true)}
    }

    publishers {
        archiveJunit('*.xml') {
            allowEmptyResults()
        }
        wsCleanup {
            includePattern('**')
            deleteDirectories(true)
            setFailBuild(false)
        }
        buildDescription('', 'master-ci')
    }
}
