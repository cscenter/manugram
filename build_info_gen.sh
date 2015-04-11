#!/bin/bash
pushd $1 >/dev/null
SHA1=$(git log -1 --pretty=%h)
TIME=$(git log -1 --pretty=%ad --date=iso || exit 1)
popd 2>&1
if [ "$SHA1" == "" ]; then exit 1; fi
function gen {
  echo "#include \"build_info.h\""
  echo "const char *GIT_LAST_SHA1 = \"$SHA1\";"
  echo "const char *GIT_LAST_TIME = \"$TIME\";"
}
gen >build_info.cpp 2>&1
