#!/bin/bash
set -e

# make forest https
export HHCM_FOREST_CLONE_DEFAULT_PROTO=https

# create ws and source it
sudo chown user:user test_ws
mkdir -p test_ws && cd test_ws
forest init

# setup env
source /opt/ros/noetic/setup.bash
source setup.bash

# add recipes
forest add-recipes git@github.com:advrhumanoids/multidof_recipes.git -t master

# build
export PYTHONUNBUFFERED=1
forest grow pybind11 --verbose --clone-depth 1 -j ${FOREST_JOBS:-1}
forest grow matlogger2 --verbose --clone-depth 1 -j ${FOREST_JOBS:-1}

# build tests
cd build/matlogger2
cmake -DBUILD_TESTS=1 .
make -j ${FOREST_JOBS:-1}