source /usr/local/sdkman/bin/sdkman-init.sh

export MAVEN_HOME=/usr/local/sdkman/candidates/maven/current/
export JAVA_HOME=/usr/local/sdkman/candidates/java/current/
export ANT_HOME=/usr/local/sdkman/candidates/ant/current/
export GRADLE_HOME=/usr/local/sdkman/candidates/gradle/current/
export NODE_HOME=/usr/local/node
export PROTOBUF_HOME=/opt/protobuf
export CCACHE=/usr/lib/ccache/
export PATH="${CCACHE}:${MAVEN_HOME}/bin:${PROTOBUF_HOME}/bin:${ANT_HOME}/bin:${GRADLE_HOME}/bin:${NODE_HOME}/bin:${PATH}"

export GRADLE_OPTS="-Dorg.gradle.daemon=true"
