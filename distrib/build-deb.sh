#!/bin/bash
cp -f ../build/release/bibot bibot-deb/usr/local/bin/
cp -f ../mstrategy.py bibot-deb/usr/local/bin/
strip bibot-deb/usr/local/bin/bibot
fakeroot dpkg-deb --build bibot-deb
mv bibot-deb.deb bibot_1.3-2_amd64.deb