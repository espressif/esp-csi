#!/bin/sh

EXPECT_IDF_VERSION=$1

CUR_IDF_VERSION=`git -C ${IDF_PATH} show -s --pretty=format:'%H'`
echo "Currect esp-idf version is ${CUR_IDF_VERSION}"

if [[ ${EXPECT_IDF_VERSION} != ${CUR_IDF_VERSION} ]]; then
    echo "** Warning **"
    echo "The version of esp-idf is not expected, please set it to ${EXPECT_IDF_VERSION}, otherwise it may not run correctly"
fi

