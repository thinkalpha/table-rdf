#!/usr/bin/env bash
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

cd ${SCRIPT_DIR}
mkdir -p temp/rdf
cp CMakeLists.txt table-rdf-config.cmake temp
cp *.h *.tpp temp/rdf

tar -cvzf table-rdf.tar.gz -C temp rdf/ CMakeLists.txt table-rdf-config.cmake
rm -rf temp

sha512sum table-rdf.tar.gz | tee table-rdf.tar.gz.sha512sum