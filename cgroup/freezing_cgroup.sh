#!/bin/bash

read -p "Enter cgroup name: "  name

echo FROZEN > /sys/fs/cgroup/freezer/$name/freezer.state

