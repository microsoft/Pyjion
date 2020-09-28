echo Building CPython in debug mode for x64 ...
cd Python/
./configure --with-pydebug
make -s -j2
cd ..