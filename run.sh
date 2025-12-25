#!/bin/bash

export ACCESS_KEY_ID=
export SECRET_ACCESS_KEY=

export CONFIG_ACCESS_KEY_ID=$ACCESS_KEY_ID
export CONFIG_SECRET_ACCESS_KEY=$SECRET_ACCESS_KEY

cd build/debug-asan

hw_obs_bench