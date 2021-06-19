#!/bin/bash

read -p "Enter pid: "  pid
read -p "Enter cgroup name: "  name

echo $pid > /sys/fs/cgroup/freezer/$name/tasks