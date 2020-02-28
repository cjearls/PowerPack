#!/bin/bash
./configure CPPFLAGS="-I/usr/local/cuda-10.2/include" LDFLAGS="-L/usr/local/cuda-10.2/targets/x86_64-linux/lib/ -lOpenCL" --with-opencl

