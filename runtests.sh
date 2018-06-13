mkdir -p Python/Pyjion
cp Pyjion/pyjion.so Python/Pyjion
cd Python


../Test/test.o
[ $? -eq 0 ] || exit $?
../Tests/tests.o
[ $? -eq 0 ] || exit $?

