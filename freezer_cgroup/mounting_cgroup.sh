#!/bin/bash

read -p "Enter cgroup name: "  name

mkdir /sys/fs/cgroup/freezer
mount -t cgroup -ofreezer freezer /sys/fs/cgroup/freezer
mkdir /sys/fs/cgroup/freezer/$name