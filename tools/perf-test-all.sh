# Copyright 2014 George King.
# Permission to use this file is granted in ploy/license.txt.

set -e
cd $(dirname "$0")/..

paths="$@"
if [[ -z "$paths" ]]; then
  paths="test/4-perf/*"
fi

for lang in c py ploy; do
  "tools/perf-test-$lang.sh" $paths
done
