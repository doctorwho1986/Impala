#!/bin/bash
# Copyright 2014 Cloudera Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# Loads a hive metastore snapshot file to re-create its postgres database.
# A metastore snapshot file is produced as an artifact of a successful
# full data load build.
#
# NOTE: Running this script will remove your existing test-warehouse directory. Be sure
# to backup any data you need before running this script.
. ${IMPALA_HOME}/bin/impala-config.sh > /dev/null 2>&1

# Always run in Debug mode.
set -x

if [[ ! $1 ]]; then
  echo "Usage: load-metastore-snapshot.sh [<metastore_snapshot_file>]"
  exit 1
fi

SNAPSHOT_FILE=$1
if [ ! -f ${SNAPSHOT_FILE} ]; then
  echo "Metastore Snapshot file '${SNAPSHOT_FILE}' not found"
  exit 1
fi

# The snapshot file has jenkins as the default user, search and replace with the current
# user (this is only useful for local environments).
# TODO: While this is safe at the moment, there is no guarentee that it will remain so.
# We're at risk is a table/column name has the string 'jenkins' in it. Find a robust way
# to do the transformation.
if [ ${USER} != "jenkins" ]; then
  echo "Searching and replacing jenkins with ${USER}"
  sed -i "s/jenkins/${USER}/g" ${SNAPSHOT_FILE}
fi

# Fail if any of these actions don't succeed.
set -e

# Drop and re-create the hive metastore database
dropdb -U hiveuser hive_impala
createdb -U hiveuser hive_impala
# Copy the contents of the SNAPSHOT_FILE
psql -U hiveuser hive_impala < ${SNAPSHOT_FILE} > /dev/null 2>&1
# Two tables (tpch.nation and functional.alltypestiny) have cache_directive_id set in
# their metadata. These directives are now stale, and will cause any query that attempts
# to cache the data in the tables to fail.
psql -U hiveuser -d hive_impala -c \
  "delete from \"TABLE_PARAMS\" where \"PARAM_KEY\"='cache_directive_id'"
psql -U hiveuser -d hive_impala -c \
  "delete from \"PARTITION_PARAMS\" where \"PARAM_KEY\"='cache_directive_id'"
