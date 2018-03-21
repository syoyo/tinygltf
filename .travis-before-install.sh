#!/bin/bash

if [[ "$TRAVIS_OS_NAME" == "osx" ]]
then
    brew upgrade
    curl -o premake5.tar.gz https://github.com/premake/premake-core/releases/download/v5.0.0-alpha12/premake-5.0.0-alpha12-macosx.tar.gz
else
    wget https://github.com/premake/premake-core/releases/download/v5.0.0-alpha12/premake-5.0.0-alpha12-linux.tar.gz -O premake5.tar.gz
fi
tar xzf premake5.tar.gz
