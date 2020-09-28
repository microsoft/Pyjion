echo Building CPython in debug mode for x64 ...
pushd Python/
./configure --with-pydebug
make -s -j2
popd