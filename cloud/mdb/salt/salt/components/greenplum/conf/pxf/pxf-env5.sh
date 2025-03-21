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

PXF_CONF="$( cd "$( dirname "${BASH_SOURCE[0]}" )/.." && pwd )"

# Path to JAVA
JAVA_HOME={{ java_home }}

# Path to Log directory
# export PXF_LOGDIR="${PXF_CONF}/logs"

# Memory
# export PXF_JVM_OPTS="-Xmx2g -Xms1g"

# Threads
# export PXF_MAX_THREADS="200"

# Kerberos path to keytab file owned by pxf service with permissions 0400
# Deprecation notice: PXF_KEYTAB will be deprecated in a future release of PXF.
#                     A per-server configuration was introduced to support
#                     multiple kerberized servers. Configuring the keytab path
#                     in pxf-site.xml is now preferred. PXF_KEYTAB is only
#                     supported for the default server for backwards
#                     compatibility. Please refer to the official PXF
#                     documentation for more details.
# export PXF_KEYTAB="${PXF_CONF}/keytabs/pxf.service.keytab"

# Kerberos principal pxf service should use. _HOST is replaced automatically with hostnames FQDN
# Deprecation notice: PXF_PRINCIPAL will be deprecated in a future release of
#                     PXF. A per-server configuration was introduced to support
#                     multiple kerberized servers. Configuring the principal
#                     name in pxf-site.xml is now preferred. PXF_PRINCIPAL is
#                     only supported for the default server for backwards
#                     compatibility. Please refer to the official PXF
#                     documentation for more details.
# export PXF_PRINCIPAL="gpadmin/_HOST@EXAMPLE.COM"

# End-user identity impersonation, set to true to enable
# Deprecation notice: PXF_USER_IMPERSONATION will be deprecated in a future
#                     release of PXF. A per-server configuration was introduced
#                     to support multiple servers. Configuring user
#                     impersonation in pxf-site.xml is now preferred. Please
#                     refer to the official PXF documentation for more details.
# export PXF_USER_IMPERSONATION=true

# Fragmenter cache, set to false to disable
# export PXF_FRAGMENTER_CACHE=true

# Kill PXF on OutOfMemoryError, set to false to disable
# export PXF_OOM_KILL=true

# Dump heap on OutOfMemoryError, set to dump path to enable
# export PXF_OOM_DUMP_PATH=/tmp/pxf_heap_dump

# Additional native libraries to be loaded by PXF
# export LD_LIBRARY_PATH=
export PXF_JVM_OPTS="-Xmx{{ pxfvars.xmx_size }} -Xms{{ pxfvars.xms_size }} -Duser.timezone={{ pxfvars.user_tz }}"
export PXF_LOGDIR="{{ gpdbvars.gplog }}"
