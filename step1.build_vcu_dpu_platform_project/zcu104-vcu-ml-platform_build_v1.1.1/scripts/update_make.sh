#!/bin/bash

find . -name makefile -exec bash -x ${PROJ_DIR}/zcu104-vcu-ml-build-example/scripts/file_update.sh '{}' \;
find . -name subdir.mk -exec bash -x ${PROJ_DIR}/zcu104-vcu-ml-build-example/scripts/file_update.sh '{}' \;

