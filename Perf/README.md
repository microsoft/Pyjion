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
C:\Source\benchmarks>py -3 perf.py -r --csv ..\Pyjion5\Perf\2016-04-05_958da51c68cdaaaefa71c7b4eb319d30f3ddcadd.csv --benchmarks=all,-chameleon_v2, C:\Source\Pyjion5\Python\PCbuild\nojit\python.exe C:\Source\Pyjion5\Python\PCbuild\amd64\python.exe
INFO:root:Automatically selected timer: perf_counter
INFO:root:Skipping benchmark slowpickle; not compatible with Python 3.5
INFO:root:Skipping benchmark rietveld; not compatible with Python 3.5
INFO:root:Skipping benchmark bzr_startup; not compatible with Python 3.5
INFO:root:Skipping benchmark pybench; not compatible with Python 3.5
INFO:root:Skipping benchmark spambayes; not compatible with Python 3.5
INFO:root:Skipping benchmark slowspitfire; not compatible with Python 3.5
INFO:root:Skipping benchmark html5lib; not compatible with Python 3.5
INFO:root:Skipping benchmark slowunpickle; not compatible with Python 3.5
INFO:root:Skipping benchmark hg_startup; not compatible with Python 3.5
INFO:root:Skipping benchmark html5lib_warmup; not compatible with Python 3.5
[ 1/42] 2to3...
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\amd64\python.exe lib3/2to3/2to3 -f all lib/2to3`
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\amd64\python.exe lib3/2to3/2to3 -f all lib/2to3` 5 times
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\nojit\python.exe lib3/2to3/2to3 -f all lib/2to3`
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\nojit\python.exe lib3/2to3/2to3 -f all lib/2to3` 5 times
[ 2/42] call_method...
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\amd64\python.exe performance/bm_call_method.py -n 300 --timer perf_counter`
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\nojit\python.exe performance/bm_call_method.py -n 300 --timer perf_counter`
[ 3/42] call_method_slots...
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\amd64\python.exe performance/bm_call_method_slots.py -n 300 --timer perf_counter`
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\nojit\python.exe performance/bm_call_method_slots.py -n 300 --timer perf_counter`
[ 4/42] call_method_unknown...
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\amd64\python.exe performance/bm_call_method_unknown.py -n 300 --timer perf_counter`
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\nojit\python.exe performance/bm_call_method_unknown.py -n 300 --timer perf_counter`
[ 5/42] call_simple...
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\amd64\python.exe performance/bm_call_simple.py -n 300 --timer perf_counter`
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\nojit\python.exe performance/bm_call_simple.py -n 300 --timer perf_counter`
[ 6/42] chaos...
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\amd64\python.exe performance/bm_chaos.py -n 100 --timer perf_counter`
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\nojit\python.exe performance/bm_chaos.py -n 100 --timer perf_counter`
[ 7/42] django_v3...
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\amd64\python.exe performance/bm_django_v3.py -n 100 --timer perf_counter`
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\nojit\python.exe performance/bm_django_v3.py -n 100 --timer perf_counter`
[ 8/42] etree_generate...
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\amd64\python.exe performance/bm_elementtree.py -n 100 --timer perf_counter generate`
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\nojit\python.exe performance/bm_elementtree.py -n 100 --timer perf_counter generate`
[ 9/42] etree_iterparse...
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\amd64\python.exe performance/bm_elementtree.py -n 100 --timer perf_counter iterparse`
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\nojit\python.exe performance/bm_elementtree.py -n 100 --timer perf_counter iterparse`
[10/42] etree_parse...
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\amd64\python.exe performance/bm_elementtree.py -n 100 --timer perf_counter parse`
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\nojit\python.exe performance/bm_elementtree.py -n 100 --timer perf_counter parse`
[11/42] etree_process...
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\amd64\python.exe performance/bm_elementtree.py -n 100 --timer perf_counter process`
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\nojit\python.exe performance/bm_elementtree.py -n 100 --timer perf_counter process`
[12/42] fannkuch...
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\amd64\python.exe performance/bm_fannkuch.py -n 100 --timer perf_counter`
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\nojit\python.exe performance/bm_fannkuch.py -n 100 --timer perf_counter`
[13/42] fastpickle...
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\amd64\python.exe performance/bm_pickle.py -n 100 --timer perf_counter --use_cpickle pickle`
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\nojit\python.exe performance/bm_pickle.py -n 100 --timer perf_counter --use_cpickle pickle`
[14/42] fastunpickle...
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\amd64\python.exe performance/bm_pickle.py -n 100 --timer perf_counter --use_cpickle unpickle`
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\nojit\python.exe performance/bm_pickle.py -n 100 --timer perf_counter --use_cpickle unpickle`
[15/42] float...
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\amd64\python.exe performance/bm_float.py -n 100 --timer perf_counter`
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\nojit\python.exe performance/bm_float.py -n 100 --timer perf_counter`
[16/42] formatted_logging...
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\amd64\python.exe performance/bm_logging.py -n 100 --timer perf_counter formatted_output`
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\nojit\python.exe performance/bm_logging.py -n 100 --timer perf_counter formatted_output`
[17/42] go...
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\amd64\python.exe performance/bm_go.py -n 100 --timer perf_counter`
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\nojit\python.exe performance/bm_go.py -n 100 --timer perf_counter`
[18/42] hexiom2...
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\amd64\python.exe performance/bm_hexiom2.py -n 4 --timer perf_counter`
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\nojit\python.exe performance/bm_hexiom2.py -n 4 --timer perf_counter`
[19/42] json_dump_v2...
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\amd64\python.exe performance/bm_json_v2.py -n 100 --timer perf_counter`
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\nojit\python.exe performance/bm_json_v2.py -n 100 --timer perf_counter`
[20/42] json_load...
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\amd64\python.exe performance/bm_json.py -n 100 --timer perf_counter json_load`
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\nojit\python.exe performance/bm_json.py -n 100 --timer perf_counter json_load`
[21/42] mako_v2...
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\amd64\python.exe performance/bm_mako_v2.py -n 1000 --timer perf_counter`
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\nojit\python.exe performance/bm_mako_v2.py -n 1000 --timer perf_counter`
[22/42] meteor_contest...
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\amd64\python.exe performance/bm_meteor_contest.py -n 100 --timer perf_counter`
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\nojit\python.exe performance/bm_meteor_contest.py -n 100 --timer perf_counter`
[23/42] nbody...
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\amd64\python.exe performance/bm_nbody.py -n 100 --timer perf_counter`
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\nojit\python.exe performance/bm_nbody.py -n 100 --timer perf_counter`
[24/42] normal_startup...
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\amd64\python.exe -c ` 2000 times
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\nojit\python.exe -c ` 2000 times
[25/42] nqueens...
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\amd64\python.exe performance/bm_nqueens.py -n 100 --timer perf_counter`
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\nojit\python.exe performance/bm_nqueens.py -n 100 --timer perf_counter`
[26/42] pathlib...
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\amd64\python.exe performance/bm_pathlib.py -n 1000 --timer perf_counter`
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\nojit\python.exe performance/bm_pathlib.py -n 1000 --timer perf_counter`
[27/42] pickle_dict...
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\amd64\python.exe performance/bm_pickle.py -n 100 --timer perf_counter --use_cpickle pickle_dict`
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\nojit\python.exe performance/bm_pickle.py -n 100 --timer perf_counter --use_cpickle pickle_dict`
[28/42] pickle_list...
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\amd64\python.exe performance/bm_pickle.py -n 100 --timer perf_counter --use_cpickle pickle_list`
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\nojit\python.exe performance/bm_pickle.py -n 100 --timer perf_counter --use_cpickle pickle_list`
[29/42] pidigits...
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\amd64\python.exe performance/bm_pidigits.py -n 100 --timer perf_counter`
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\nojit\python.exe performance/bm_pidigits.py -n 100 --timer perf_counter`
[30/42] raytrace...
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\amd64\python.exe performance/bm_raytrace.py -n 100 --timer perf_counter`
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\nojit\python.exe performance/bm_raytrace.py -n 100 --timer perf_counter`
[31/42] regex_compile...
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\amd64\python.exe performance/bm_regex_compile.py -n 100 --timer perf_counter`
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\nojit\python.exe performance/bm_regex_compile.py -n 100 --timer perf_counter`
[32/42] regex_effbot...
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\amd64\python.exe performance/bm_regex_effbot.py -n 100 --timer perf_counter`
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\nojit\python.exe performance/bm_regex_effbot.py -n 100 --timer perf_counter`
[33/42] regex_v8...
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\amd64\python.exe performance/bm_regex_v8.py -n 100 --timer perf_counter`
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\nojit\python.exe performance/bm_regex_v8.py -n 100 --timer perf_counter`
[34/42] richards...
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\amd64\python.exe performance/bm_richards.py -n 100 --timer perf_counter`
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\nojit\python.exe performance/bm_richards.py -n 100 --timer perf_counter`
[35/42] silent_logging...
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\amd64\python.exe performance/bm_logging.py -n 100 --timer perf_counter no_output`
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\nojit\python.exe performance/bm_logging.py -n 100 --timer perf_counter no_output`
[36/42] simple_logging...
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\amd64\python.exe performance/bm_logging.py -n 100 --timer perf_counter simple_output`
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\nojit\python.exe performance/bm_logging.py -n 100 --timer perf_counter simple_output`
[37/42] spectral_norm...
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\amd64\python.exe performance/bm_spectral_norm.py -n 100 --timer perf_counter`
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\nojit\python.exe performance/bm_spectral_norm.py -n 100 --timer perf_counter`
[38/42] startup_nosite...
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\amd64\python.exe -S -c ` 4000 times
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\nojit\python.exe -S -c ` 4000 times
[39/42] telco...
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\amd64\python.exe performance/bm_telco.py -n 100 --timer perf_counter`
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\nojit\python.exe performance/bm_telco.py -n 100 --timer perf_counter`
[40/42] tornado_http...
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\amd64\python.exe performance/bm_tornado_http.py -n 200 --timer perf_counter`
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\nojit\python.exe performance/bm_tornado_http.py -n 200 --timer perf_counter`
[41/42] unpack_sequence...
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\amd64\python.exe performance/bm_unpack_sequence.py -n 100000 --timer perf_counter`
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\nojit\python.exe performance/bm_unpack_sequence.py -n 100000 --timer perf_counter`
[42/42] unpickle_list...
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\amd64\python.exe performance/bm_pickle.py -n 100 --timer perf_counter --use_cpickle unpickle_list`
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\nojit\python.exe performance/bm_pickle.py -n 100 --timer perf_counter --use_cpickle unpickle_list`

Report on Windows dinov1  10.0.10586 AMD64 Intel64 Family 6 Model 44 Stepping 2, GenuineIntel
Total CPU cores: 8

### formatted_logging ###
Min: 0.757906 -> 0.726722: 1.04x faster
Avg: 0.769573 -> 0.738261: 1.04x faster
Significant (t=49.77)
Stddev: 0.00426 -> 0.00463: 1.0885x larger

### normal_startup ###
Min: 0.929839 -> 1.043941: 1.12x slower
Avg: 0.939317 -> 1.056562: 1.12x slower
Significant (t=-90.28)
Stddev: 0.00491 -> 0.01202: 2.4475x larger

### simple_logging ###
Min: 0.663767 -> 0.644777: 1.03x faster
Avg: 0.673285 -> 0.657034: 1.02x faster
Significant (t=29.23)
Stddev: 0.00340 -> 0.00440: 1.2966x larger

### startup_nosite ###
Min: 0.615553 -> 0.716569: 1.16x slower
Avg: 0.626866 -> 0.737565: 1.18x slower
Significant (t=-22.08)
Stddev: 0.01831 -> 0.06850: 3.7413x larger

### unpickle_list ###
Min: 0.598304 -> 0.620273: 1.04x slower
Avg: 0.616499 -> 0.636750: 1.03x slower
Significant (t=-21.20)
Stddev: 0.00821 -> 0.00489: 1.6797x smaller

The following not significant results are hidden, use -v to show them:
2to3, call_method, call_method_slots, call_method_unknown, call_simple, chaos, django_v3, etree_generate, etree_iterparse, etree_parse, etree_process, fannkuch, fastpickle, fastunpickle, float, go, hexiom2, json_dump_v2, json_load, mako_v2, meteor_contest, nbody, nqueens, pathlib, pickle_dict, pickle_list, pidigits, raytrace, regex_compile, regex_effbot, regex_v8, richards, silent_logging, spectral_norm, telco, tornado_http, unpack_sequence.
```
