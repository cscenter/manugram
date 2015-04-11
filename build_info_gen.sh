#!/bin/bash
pushd $1 >/dev/null
GIT_SHA1=$(git log -1 --pretty=%H)
GIT_TIME=$(git log -1 --pretty=%ad --date=iso || exit 1)
TIME=$(date +'%Y-%m-%d %H:%M:%S %z')
popd >/dev/null
if [ "$GIT_SHA1" == "" ]; then exit 1; fi

function gen {
  echo "#include \"build_info.h\""
  echo "const char *GIT_LAST_SHA1 = \"$GIT_SHA1\";"
  echo "const char *GIT_LAST_TIME = \"$GIT_TIME\";"
  echo "const char *BUILD_TIME = \"$TIME\";"
}
gen >build_info.cpp
