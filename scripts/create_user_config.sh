#!/bin/bash

SOURCE_CODE_PATH=../rancilio-pid/
USER_CONFIG_FILE_NAME=userConfig

echo "Creating user config file from sample"

cp -v ${SOURCE_CODE_PATH}${USER_CONFIG_FILE_NAME}_sample.h ${SOURCE_CODE_PATH}${USER_CONFIG_FILE_NAME}.h

if [[ "$OSTYPE" == "linux-gnu"* ]]; then
        SED_COMMAND=sed
elif [[ "$OSTYPE" == "darwin"* ]]; then
        SED_COMMAND=gsed
        if ! command -v ${SED_COMMAND} &> /dev/null
        then
            echo "gnu-sed could not be found, install it via brew install gnu-sed"
            exit
        fi
fi

echo Please enter Blynk authentification code
read BLYNK_AUTH

echo Please enter desired hostname
read HOSTNAME

${SED_COMMAND} -i "s/blynk_auth/${BLYNK_AUTH}/" ${SOURCE_CODE_PATH}${USER_CONFIG_FILE_NAME}.h
${SED_COMMAND} -i "s/ota_hostname/${HOSTNAME}/" ${SOURCE_CODE_PATH}${USER_CONFIG_FILE_NAME}.h

