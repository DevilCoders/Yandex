{%- set java_home = salt['cmd.shell']('echo $(dirname $(dirname $(readlink $(readlink $(which javac)))))') -%}
{% from "components/greenplum/map.jinja" import pxfvars,gpdbvars with context %}
#!/bin/bash

##############################################################################
# This file contains PXF properties that can be specified by users           #
# to customize their deployments. This file is sourced by PXF Server control #
# scripts upon initialization, start and stop of the PXF Server.             #
#                                                                            #
# To update a property, uncomment the line and provide a new value.          #
##############################################################################

# Path to JAVA
export JAVA_HOME={{ java_home }}

# Path to Log directory
# export PXF_LOGDIR="${PXF_BASE}/logs"
export PXF_LOGDIR="{{ gpdbvars.gplog }}"

# Path to Run directory
# export PXF_RUNDIR=${PXF_RUNDIR:=${PXF_BASE}/run}

# Memory
# export PXF_JVM_OPTS="-Xmx2g -Xms1g"
export PXF_JVM_OPTS="-Xmx{{ pxfvars.xmx_size }} -Xms{{ pxfvars.xms_size }} -Duser.timezone={{ pxfvars.user_tz }} -Djava.io.tmpdir={{ pxfvars.java_io_tmpdir }}"

# Kill PXF on OutOfMemoryError, set to false to disable
# export PXF_OOM_KILL=true

# Dump heap on OutOfMemoryError, set to dump path to enable
# export PXF_OOM_DUMP_PATH=${PXF_BASE}/run/pxf_heap_dump

# Additional locations to be class-loaded by PXF
# export PXF_LOADER_PATH=

# Additional native libraries to be loaded by PXF
# export LD_LIBRARY_PATH=
