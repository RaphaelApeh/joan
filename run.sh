#!/bin/bash

FILE=%1
cd build || exit

eval "joan ../examples/$FILE"