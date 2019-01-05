#!/bin/bash

platform=`uname`
echo "Setting up $platform for goat development"

TOOLCHAIN=$PWD/xtensa-esp32-elf/bin
ESP_IDF=$PWD/idf

if [[ $platform = "Linux" ]]; then

    if [ ! -d "$TOOLCHAIN" ]; then
        wget -q https://dl.espressif.com/dl/xtensa-esp32-elf-linux64-1.22.0-80-g6c4433a-5.2.0.tar.gz
        tar -xzf xtensa-esp32-elf-linux64-1.22.0-80-g6c4433a-5.2.0.tar.gz
        rm xtensa-esp32-elf-linux64-1.22.0-80-g6c4433a-5.2.0.tar.gz
    fi

elif [[ $platform = "Darwin" ]]; then

    if [ ! -d "$TOOLCHAIN" ]; then
        wget -q https://dl.espressif.com/dl/xtensa-esp32-elf-osx-1.22.0-80-g6c4433a-5.2.0.tar.gz
        tar -xzf xtensa-esp32-elf-osx-1.22.0-80-g6c4433a-5.2.0.tar.gz
        rm xtensa-esp32-elf-osx-1.22.0-80-g6c4433a-5.2.0.tar.gz
    fi

fi

if [[ $PATH != *$TOOLCHAIN* ]]; then
    export PATH=$PATH:$TOOLCHAIN
    echo "Added toolchain to path"
fi

python -m pip install --user -r $ESP_IDF/requirements.txt

export IDF_PATH=$ESP_IDF

echo "Done"
