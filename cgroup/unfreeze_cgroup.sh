#!/bin/bash

read -p "Enter cgroup name: "  name

 echo THAWED > /sys/fs/cgroup/freezer/$name/freezer.state