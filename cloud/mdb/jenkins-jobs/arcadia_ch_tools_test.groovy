job("arcadia-ch-tools-test"){
    description("""
<h2>Test time</h2>
<br/>
<img src='https://jenkins.db.yandex-team.ru/job/arcadia-ch-tools-test/buildTimeGraph/png' />""")
    logRotator(14, 200, 14, 200)
    concurrentBuild()
    label('dbaas')

    authenticationToken('token')

    parameters {
        stringParam('ARCANUMID', 'trunk')
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
            string('YAV_OAUTH', 'robot-pgaas-ci-yav_oauth')
        }
        buildName('${ENV,var="ARCANUMID"} ${BUILD_NUMBER}')
        sshAgent('908ba07a-7d93-4cf1-9bb6-bce7533cb93c')
    }

    steps {
        shell('env')
        shell('flock -s /tmp/arcadia.lock rsync -a /home/robot-pgaas-ci/arcadia/ arcadia/')
        shell('cd arcadia && retry timeout 600 svn up && retry timeout 600 ./ya make -j0 --checkout cloud/mdb/clickhouse')
        shell('cd arcadia && echo "Arcanum ID: $ARCANUMID" && if [ "$ARCANUMID" != "trunk" ]; then retry bash -c "(./ya unshelve -d -a $ARCANUMID || ./ya unshelve -a $ARCANUMID)"; fi')
        shell('cd arcadia && retry timeout 600 ./ya make -j0 --checkout cloud/mdb/clickhouse')
        shell('cd arcadia/cloud/mdb/clickhouse/tools/tests && PATH=$WORKSPACE/arcadia:$PATH make lint')
        shell('''
python3 -c "import yaml; print('VERSIONS=' + ','.join(v['version'] for v in yaml.safe_load(open('arcadia/cloud/mdb/clickhouse/versions/versions.yaml'))))" > versions.properties
''')
        systemGroovyCommand('''
import jenkins.*
import jenkins.model.*
import hudson.*
import hudson.model.*

def build = Thread.currentThread().executable
def resolver = build.buildVariableResolver
def vars = build.getEnvVars()

def channel = Jenkins.getInstance().getComputer(vars["NODE_NAME"]).getChannel()
properties_file = new FilePath(channel, build.workspace.toString() + "/versions.properties")
properties = new Properties()
properties.load(properties_file.read())

def futures = []
for (ch_version in properties.VERSIONS.split(",")) {
  def job_name = "arcadia-ch-tools-test-func"
  println "Triggering downstream job $job_name for ClickHouse version $ch_version"
  def job = Hudson.instance.getJob(job_name)

  def params = [
    new StringParameterValue("ARCANUMID", resolver.resolve("ARCANUMID")),
    new StringParameterValue("trigger_id", resolver.resolve("trigger_id")),
    new StringParameterValue("CLICKHOUSE_VERSION", ch_version),
  ]

  futures << job.scheduleBuild2(0, new Cause.UpstreamCause(build), new ParametersAction(params))
}

println("Waiting for downstream jobs to complete")

should_wait = true

deadline = System.currentTimeMillis() + 14400000L

while (should_wait) {
  done_futures = 0
  for (future in futures) {
    if (future.isDone()) {
      job = future.get()
      if (job.result != Result.SUCCESS) {
        throw new AbortException("${job.getAbsoluteUrl()} failed")
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
