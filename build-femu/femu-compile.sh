#!/bin/bash

NRCPUS="$(cat /proc/cpuinfo | grep "vendor_id" | wc -l)"

../configure --enable-kvm --target-list=x86_64-softmmu --extra-cflags=-Wno-error --extra-cflags=-w

make clean

make -j $NRCPUS


