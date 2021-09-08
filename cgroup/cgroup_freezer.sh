#!/bin/bash

read -p "Enter cgroup name: "  name
read -p "Enter pid: "  pid

mkdir /sys/fs/cgroup/freezer
mount -t cgroup -ofreezer freezer /sys/fs/cgroup/freezer
mkdir /sys/fs/cgroup/freezer/$name

echo $pid > /sys/fs/cgroup/freezer/$name/tasks
echo FROZEN > /sys/fs/cgroup/freezer/$name/freezer.state

sleep 5
echo THAWED > /sys/fs/cgroup/freezer/$name/freezer.state

