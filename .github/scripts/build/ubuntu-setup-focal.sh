#!/bin/bash

apt-get -y update
apt-get -y install build-essential python3-pip

pip install meson ninja
