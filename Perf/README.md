# Benchmarking Performance

## About
This directory holds results from running the
[Python Unified Benchmark Suite](https://hg.python.org/benchmarks) against
Pyjion using a `Release` build. The CSV files are the data that `perf.py`
emits. The file names include the date the benchmarks were run along with the
Git commit hash that represents the build of Pyjion used (as found by
`git log -p -1`).

## Latest run
```
PS C:\Users\brcan\Documents\Repositories\py-benchmarks> py -3 perf.py -r --csv ..\Pyjion\Perf\2016-03-31_4580f9497fc4cbb8e76e37c604914a1e1c0f898c.csv ..\cpython-ref\PCbuild\amd64\python.exe ..\Pyjion\Python\PCbuild\amd64\python.exe
INFO:root:Automatically selected timer: perf_counter
[ 1/10] 2to3...
INFO:root:Running `..\Pyjion\Python\PCbuild\amd64\python.exe lib3/2to3/2to3 -f all lib/2to3`
INFO:root:Running `..\Pyjion\Python\PCbuild\amd64\python.exe lib3/2to3/2to3 -f all lib/2to3` 5 times
INFO:root:Running `..\cpython-ref\PCbuild\amd64\python.exe lib3/2to3/2to3 -f all lib/2to3`
INFO:root:Running `..\cpython-ref\PCbuild\amd64\python.exe lib3/2to3/2to3 -f all lib/2to3` 5 times
[ 2/10] chameleon_v2...
INFO:root:Running `..\Pyjion\Python\PCbuild\amd64\python.exe performance/bm_chameleon_v2.py -n 100 --timer perf_counter`
INFO:root:Running `..\cpython-ref\PCbuild\amd64\python.exe performance/bm_chameleon_v2.py -n 100 --timer perf_counter`
[ 3/10] django_v3...
INFO:root:Running `..\Pyjion\Python\PCbuild\amd64\python.exe performance/bm_django_v3.py -n 100 --timer perf_counter`
INFO:root:Running `..\cpython-ref\PCbuild\amd64\python.exe performance/bm_django_v3.py -n 100 --timer perf_counter`
[ 4/10] fastpickle...
INFO:root:Running `..\Pyjion\Python\PCbuild\amd64\python.exe performance/bm_pickle.py -n 100 --timer perf_counter --use_cpickle pickle`
INFO:root:Running `..\cpython-ref\PCbuild\amd64\python.exe performance/bm_pickle.py -n 100 --timer perf_counter --use_cpickle pickle`
[ 5/10] fastunpickle...
INFO:root:Running `..\Pyjion\Python\PCbuild\amd64\python.exe performance/bm_pickle.py -n 100 --timer perf_counter --use_cpickle unpickle`
INFO:root:Running `..\cpython-ref\PCbuild\amd64\python.exe performance/bm_pickle.py -n 100 --timer perf_counter --use_cpickle unpickle`
[ 6/10] json_dump_v2...
INFO:root:Running `..\Pyjion\Python\PCbuild\amd64\python.exe performance/bm_json_v2.py -n 100 --timer perf_counter`
INFO:root:Running `..\cpython-ref\PCbuild\amd64\python.exe performance/bm_json_v2.py -n 100 --timer perf_counter`
[ 7/10] json_load...
INFO:root:Running `..\Pyjion\Python\PCbuild\amd64\python.exe performance/bm_json.py -n 100 --timer perf_counter json_load`
INFO:root:Running `..\cpython-ref\PCbuild\amd64\python.exe performance/bm_json.py -n 100 --timer perf_counter json_load`
[ 8/10] nbody...
INFO:root:Running `..\Pyjion\Python\PCbuild\amd64\python.exe performance/bm_nbody.py -n 100 --timer perf_counter`
INFO:root:Running `..\cpython-ref\PCbuild\amd64\python.exe performance/bm_nbody.py -n 100 --timer perf_counter`
[ 9/10] regex_v8...
INFO:root:Running `..\Pyjion\Python\PCbuild\amd64\python.exe performance/bm_regex_v8.py -n 100 --timer perf_counter`
INFO:root:Running `..\cpython-ref\PCbuild\amd64\python.exe performance/bm_regex_v8.py -n 100 --timer perf_counter`
[10/10] tornado_http...
INFO:root:Running `..\Pyjion\Python\PCbuild\amd64\python.exe performance/bm_tornado_http.py -n 200 --timer perf_counter`
INFO:root:Running `..\cpython-ref\PCbuild\amd64\python.exe performance/bm_tornado_http.py -n 200 --timer perf_counter`

Report on Windows BRETTCANNON-DT 10 10.0.10586 AMD64 Intel64 Family 6 Model 63 Stepping 2, GenuineIntel
Total CPU cores: 8

### chameleon_v2 ###
Min: 3.911691 -> 4.076780: 1.04x slower
Avg: 3.957607 -> 4.128225: 1.04x slower
Significant (t=-35.64)
Stddev: 0.02841 -> 0.03854: 1.3565x larger

### json_load ###
Min: 0.349240 -> 0.357310: 1.02x slower
Avg: 0.354292 -> 0.362083: 1.02x slower
Significant (t=-8.96)
Stddev: 0.00552 -> 0.00673: 1.2192x larger

### nbody ###
Min: 0.233559 -> 0.242705: 1.04x slower
Avg: 0.235344 -> 0.244660: 1.04x slower
Significant (t=-20.02)
Stddev: 0.00333 -> 0.00325: 1.0227x smaller

### tornado_http ###
Min: 0.418463 -> 0.418276: 1.00x faster
Avg: 0.449733 -> 0.459540: 1.02x slower
Significant (t=-3.94)
Stddev: 0.02247 -> 0.02704: 1.2031x larger

The following not significant results are hidden, use -v to show them:
2to3, django_v3, fastpickle, fastunpickle, json_dump_v2, regex_v8.
```
