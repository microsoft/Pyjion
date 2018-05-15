pushd CoreCLR
./build.sh
cd ../Python
./configure
make
popd
