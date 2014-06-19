# Copyright 2013 George King.
# Permission to use this file is granted in ploy/license.txt.

set -e
cd $(dirname "$0")/..

for n in prof-res-usage text-to-c-literal; do
  sh/update-tool.sh $n
done
