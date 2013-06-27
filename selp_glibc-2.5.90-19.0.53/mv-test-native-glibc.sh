#!/usr/bin/env bash
pushd /testsuite/glibc

./test-glibc.sh 2>&1 |tee result.log

cat result.log |grep Error 2>&1 |tee fail.log

popd

