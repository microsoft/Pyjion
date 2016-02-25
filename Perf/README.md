# Benchmarking Performance

## About
This directory holds results from running the
[Python Unified Benchmark Suite](https://hg.python.org/benchmarks) against
Pyjion using a `Release` build. The CSV files are the data that `perf.py`
emits. The file names include the date the benchmarks were run along with the
Git commit hash that represents the build of Pyjion used (as found by
`git log -p -1`).

## Latest run
The `chameleon_v2` benchmark was skipped due to it failing under both CPython
and Pyjion.

```
PS C:\Users\brcan\Documents\Repositories\py-benchmarks> py -3 .\perf.py -f -b 2n3,-chameleon_v2 --csv=jit-always-all.csv ..\cpython-ref\PCbuild\amd64\python.exe ..\Pyjion\Python\PCbuild\amd64\python.e
xe
INFO:root:Automatically selected timer: perf_counter
INFO:root:Skipping slow benchmarks (hexiom2) in fast mode
[ 1/41] 2to3...
INFO:root:Running `..\Pyjion\Python\PCbuild\amd64\python.exe .\lib3/2to3/2to3 -f all .\lib3/2to3/lib2to3/refactor.py`
INFO:root:Running `..\Pyjion\Python\PCbuild\amd64\python.exe .\lib3/2to3/2to3 -f all .\lib3/2to3/lib2to3/refactor.py` 1 time
INFO:root:Running `..\cpython-ref\PCbuild\amd64\python.exe .\lib3/2to3/2to3 -f all .\lib3/2to3/lib2to3/refactor.py`
INFO:root:Running `..\cpython-ref\PCbuild\amd64\python.exe .\lib3/2to3/2to3 -f all .\lib3/2to3/lib2to3/refactor.py` 1 time
[ 2/41] call_method...
INFO:root:Running `..\Pyjion\Python\PCbuild\amd64\python.exe .\performance/bm_call_method.py -n 15 --timer perf_counter`
INFO:root:Running `..\cpython-ref\PCbuild\amd64\python.exe .\performance/bm_call_method.py -n 15 --timer perf_counter`
[ 3/41] call_method_slots...
INFO:root:Running `..\Pyjion\Python\PCbuild\amd64\python.exe .\performance/bm_call_method_slots.py -n 15 --timer perf_counter`
INFO:root:Running `..\cpython-ref\PCbuild\amd64\python.exe .\performance/bm_call_method_slots.py -n 15 --timer perf_counter`
[ 4/41] call_method_unknown...
INFO:root:Running `..\Pyjion\Python\PCbuild\amd64\python.exe .\performance/bm_call_method_unknown.py -n 15 --timer perf_counter`
INFO:root:Running `..\cpython-ref\PCbuild\amd64\python.exe .\performance/bm_call_method_unknown.py -n 15 --timer perf_counter`
[ 5/41] call_simple...
INFO:root:Running `..\Pyjion\Python\PCbuild\amd64\python.exe .\performance/bm_call_simple.py -n 15 --timer perf_counter`
INFO:root:Running `..\cpython-ref\PCbuild\amd64\python.exe .\performance/bm_call_simple.py -n 15 --timer perf_counter`
[ 6/41] chaos...
INFO:root:Running `..\Pyjion\Python\PCbuild\amd64\python.exe .\performance/bm_chaos.py -n 5 --timer perf_counter`
INFO:root:Running `..\cpython-ref\PCbuild\amd64\python.exe .\performance/bm_chaos.py -n 5 --timer perf_counter`
[ 7/41] django_v3...
INFO:root:Running `..\Pyjion\Python\PCbuild\amd64\python.exe .\performance/bm_django_v3.py -n 5 --timer perf_counter`
INFO:root:Running `..\cpython-ref\PCbuild\amd64\python.exe .\performance/bm_django_v3.py -n 5 --timer perf_counter`
[ 8/41] etree_generate...
INFO:root:Running `..\Pyjion\Python\PCbuild\amd64\python.exe .\performance/bm_elementtree.py -n 5 --timer perf_counter generate`
INFO:root:Running `..\cpython-ref\PCbuild\amd64\python.exe .\performance/bm_elementtree.py -n 5 --timer perf_counter generate`
[ 9/41] etree_iterparse...
INFO:root:Running `..\Pyjion\Python\PCbuild\amd64\python.exe .\performance/bm_elementtree.py -n 5 --timer perf_counter iterparse`
INFO:root:Running `..\cpython-ref\PCbuild\amd64\python.exe .\performance/bm_elementtree.py -n 5 --timer perf_counter iterparse`
[10/41] etree_parse...
INFO:root:Running `..\Pyjion\Python\PCbuild\amd64\python.exe .\performance/bm_elementtree.py -n 5 --timer perf_counter parse`
INFO:root:Running `..\cpython-ref\PCbuild\amd64\python.exe .\performance/bm_elementtree.py -n 5 --timer perf_counter parse`
[11/41] etree_process...
INFO:root:Running `..\Pyjion\Python\PCbuild\amd64\python.exe .\performance/bm_elementtree.py -n 5 --timer perf_counter process`
INFO:root:Running `..\cpython-ref\PCbuild\amd64\python.exe .\performance/bm_elementtree.py -n 5 --timer perf_counter process`
[12/41] fannkuch...
INFO:root:Running `..\Pyjion\Python\PCbuild\amd64\python.exe .\performance/bm_fannkuch.py -n 5 --timer perf_counter`
INFO:root:Running `..\cpython-ref\PCbuild\amd64\python.exe .\performance/bm_fannkuch.py -n 5 --timer perf_counter`
[13/41] fastpickle...
INFO:root:Running `..\Pyjion\Python\PCbuild\amd64\python.exe .\performance/bm_pickle.py -n 5 --timer perf_counter --use_cpickle pickle`
INFO:root:Running `..\cpython-ref\PCbuild\amd64\python.exe .\performance/bm_pickle.py -n 5 --timer perf_counter --use_cpickle pickle`
[14/41] fastunpickle...
INFO:root:Running `..\Pyjion\Python\PCbuild\amd64\python.exe .\performance/bm_pickle.py -n 5 --timer perf_counter --use_cpickle unpickle`
INFO:root:Running `..\cpython-ref\PCbuild\amd64\python.exe .\performance/bm_pickle.py -n 5 --timer perf_counter --use_cpickle unpickle`
[15/41] float...
INFO:root:Running `..\Pyjion\Python\PCbuild\amd64\python.exe .\performance/bm_float.py -n 5 --timer perf_counter`
INFO:root:Running `..\cpython-ref\PCbuild\amd64\python.exe .\performance/bm_float.py -n 5 --timer perf_counter`
[16/41] formatted_logging...
INFO:root:Running `..\Pyjion\Python\PCbuild\amd64\python.exe .\performance/bm_logging.py -n 5 --timer perf_counter formatted_output`
INFO:root:Running `..\cpython-ref\PCbuild\amd64\python.exe .\performance/bm_logging.py -n 5 --timer perf_counter formatted_output`
[17/41] go...
INFO:root:Running `..\Pyjion\Python\PCbuild\amd64\python.exe .\performance/bm_go.py -n 5 --timer perf_counter`
INFO:root:Running `..\cpython-ref\PCbuild\amd64\python.exe .\performance/bm_go.py -n 5 --timer perf_counter`
[18/41] json_dump_v2...
INFO:root:Running `..\Pyjion\Python\PCbuild\amd64\python.exe .\performance/bm_json_v2.py -n 5 --timer perf_counter`
INFO:root:Running `..\cpython-ref\PCbuild\amd64\python.exe .\performance/bm_json_v2.py -n 5 --timer perf_counter`
[19/41] json_load...
INFO:root:Running `..\Pyjion\Python\PCbuild\amd64\python.exe .\performance/bm_json.py -n 5 --timer perf_counter json_load`
INFO:root:Running `..\cpython-ref\PCbuild\amd64\python.exe .\performance/bm_json.py -n 5 --timer perf_counter json_load`
[20/41] mako_v2...
INFO:root:Running `..\Pyjion\Python\PCbuild\amd64\python.exe .\performance/bm_mako_v2.py -n 50 --timer perf_counter`
INFO:root:Running `..\cpython-ref\PCbuild\amd64\python.exe .\performance/bm_mako_v2.py -n 50 --timer perf_counter`
[21/41] meteor_contest...
INFO:root:Running `..\Pyjion\Python\PCbuild\amd64\python.exe .\performance/bm_meteor_contest.py -n 5 --timer perf_counter`
INFO:root:Running `..\cpython-ref\PCbuild\amd64\python.exe .\performance/bm_meteor_contest.py -n 5 --timer perf_counter`
[22/41] nbody...
INFO:root:Running `..\Pyjion\Python\PCbuild\amd64\python.exe .\performance/bm_nbody.py -n 5 --timer perf_counter`
INFO:root:Running `..\cpython-ref\PCbuild\amd64\python.exe .\performance/bm_nbody.py -n 5 --timer perf_counter`
[23/41] normal_startup...
INFO:root:Running `..\Pyjion\Python\PCbuild\amd64\python.exe -c ` 100 times
INFO:root:Running `..\cpython-ref\PCbuild\amd64\python.exe -c ` 100 times
[24/41] nqueens...
INFO:root:Running `..\Pyjion\Python\PCbuild\amd64\python.exe .\performance/bm_nqueens.py -n 5 --timer perf_counter`
INFO:root:Running `..\cpython-ref\PCbuild\amd64\python.exe .\performance/bm_nqueens.py -n 5 --timer perf_counter`
[25/41] pathlib...
INFO:root:Running `..\Pyjion\Python\PCbuild\amd64\python.exe .\performance/bm_pathlib.py -n 50 --timer perf_counter`
INFO:root:Running `..\cpython-ref\PCbuild\amd64\python.exe .\performance/bm_pathlib.py -n 50 --timer perf_counter`
[26/41] pickle_dict...
INFO:root:Running `..\Pyjion\Python\PCbuild\amd64\python.exe .\performance/bm_pickle.py -n 5 --timer perf_counter --use_cpickle pickle_dict`
INFO:root:Running `..\cpython-ref\PCbuild\amd64\python.exe .\performance/bm_pickle.py -n 5 --timer perf_counter --use_cpickle pickle_dict`
[27/41] pickle_list...
INFO:root:Running `..\Pyjion\Python\PCbuild\amd64\python.exe .\performance/bm_pickle.py -n 5 --timer perf_counter --use_cpickle pickle_list`
INFO:root:Running `..\cpython-ref\PCbuild\amd64\python.exe .\performance/bm_pickle.py -n 5 --timer perf_counter --use_cpickle pickle_list`
[28/41] pidigits...
INFO:root:Running `..\Pyjion\Python\PCbuild\amd64\python.exe .\performance/bm_pidigits.py -n 5 --timer perf_counter`
INFO:root:Running `..\cpython-ref\PCbuild\amd64\python.exe .\performance/bm_pidigits.py -n 5 --timer perf_counter`
[29/41] raytrace...
INFO:root:Running `..\Pyjion\Python\PCbuild\amd64\python.exe .\performance/bm_raytrace.py -n 5 --timer perf_counter`
INFO:root:Running `..\cpython-ref\PCbuild\amd64\python.exe .\performance/bm_raytrace.py -n 5 --timer perf_counter`
[30/41] regex_compile...
INFO:root:Running `..\Pyjion\Python\PCbuild\amd64\python.exe .\performance/bm_regex_compile.py -n 5 --timer perf_counter`
INFO:root:Running `..\cpython-ref\PCbuild\amd64\python.exe .\performance/bm_regex_compile.py -n 5 --timer perf_counter`
[31/41] regex_effbot...
INFO:root:Running `..\Pyjion\Python\PCbuild\amd64\python.exe .\performance/bm_regex_effbot.py -n 5 --timer perf_counter`
INFO:root:Running `..\cpython-ref\PCbuild\amd64\python.exe .\performance/bm_regex_effbot.py -n 5 --timer perf_counter`
[32/41] regex_v8...
INFO:root:Running `..\Pyjion\Python\PCbuild\amd64\python.exe .\performance/bm_regex_v8.py -n 5 --timer perf_counter`
INFO:root:Running `..\cpython-ref\PCbuild\amd64\python.exe .\performance/bm_regex_v8.py -n 5 --timer perf_counter`
[33/41] richards...
INFO:root:Running `..\Pyjion\Python\PCbuild\amd64\python.exe .\performance/bm_richards.py -n 5 --timer perf_counter`
INFO:root:Running `..\cpython-ref\PCbuild\amd64\python.exe .\performance/bm_richards.py -n 5 --timer perf_counter`
[34/41] silent_logging...
INFO:root:Running `..\Pyjion\Python\PCbuild\amd64\python.exe .\performance/bm_logging.py -n 5 --timer perf_counter no_output`
INFO:root:Running `..\cpython-ref\PCbuild\amd64\python.exe .\performance/bm_logging.py -n 5 --timer perf_counter no_output`
[35/41] simple_logging...
INFO:root:Running `..\Pyjion\Python\PCbuild\amd64\python.exe .\performance/bm_logging.py -n 5 --timer perf_counter simple_output`
INFO:root:Running `..\cpython-ref\PCbuild\amd64\python.exe .\performance/bm_logging.py -n 5 --timer perf_counter simple_output`
[36/41] spectral_norm...
INFO:root:Running `..\Pyjion\Python\PCbuild\amd64\python.exe .\performance/bm_spectral_norm.py -n 5 --timer perf_counter`
INFO:root:Running `..\cpython-ref\PCbuild\amd64\python.exe .\performance/bm_spectral_norm.py -n 5 --timer perf_counter`
[37/41] startup_nosite...
INFO:root:Running `..\Pyjion\Python\PCbuild\amd64\python.exe -S -c ` 200 times
INFO:root:Running `..\cpython-ref\PCbuild\amd64\python.exe -S -c ` 200 times
[38/41] telco...
INFO:root:Running `..\Pyjion\Python\PCbuild\amd64\python.exe .\performance/bm_telco.py -n 5 --timer perf_counter`
INFO:root:Running `..\cpython-ref\PCbuild\amd64\python.exe .\performance/bm_telco.py -n 5 --timer perf_counter`
[39/41] tornado_http...
INFO:root:Running `..\Pyjion\Python\PCbuild\amd64\python.exe .\performance/bm_tornado_http.py -n 10 --timer perf_counter`
INFO:root:Running `..\cpython-ref\PCbuild\amd64\python.exe .\performance/bm_tornado_http.py -n 10 --timer perf_counter`
[40/41] unpack_sequence...
INFO:root:Running `..\Pyjion\Python\PCbuild\amd64\python.exe .\performance/bm_unpack_sequence.py -n 5000 --timer perf_counter`
INFO:root:Running `..\cpython-ref\PCbuild\amd64\python.exe .\performance/bm_unpack_sequence.py -n 5000 --timer perf_counter`
[41/41] unpickle_list...
INFO:root:Running `..\Pyjion\Python\PCbuild\amd64\python.exe .\performance/bm_pickle.py -n 5 --timer perf_counter --use_cpickle unpickle_list`
INFO:root:Running `..\cpython-ref\PCbuild\amd64\python.exe .\performance/bm_pickle.py -n 5 --timer perf_counter --use_cpickle unpickle_list`

Report on Windows BRETTCANNON-DT 10 10.0.10586 AMD64 Intel64 Family 6 Model 63 Stepping 2, GenuineIntel
Total CPU cores: 8

### 2to3 ###
0.447068 -> 2.405002: 5.38x slower

### call_method ###
Min: 0.276877 -> 0.345693: 1.25x slower
Avg: 0.277831 -> 0.346999: 1.25x slower
Significant (t=-166.09)
Stddev: 0.00076 -> 0.00142: 1.8758x larger

### call_method_slots ###
Min: 0.274336 -> 0.340792: 1.24x slower
Avg: 0.274846 -> 0.341469: 1.24x slower
Significant (t=-283.13)
Stddev: 0.00061 -> 0.00068: 1.1150x larger

### call_method_unknown ###
Min: 0.284908 -> 0.384645: 1.35x slower
Avg: 0.286172 -> 0.385152: 1.35x slower
Significant (t=-291.66)
Stddev: 0.00122 -> 0.00050: 2.4377x smaller

### call_simple ###
Min: 0.211351 -> 0.258323: 1.22x slower
Avg: 0.212849 -> 0.258783: 1.22x slower
Significant (t=-121.65)
Stddev: 0.00141 -> 0.00040: 3.4950x smaller

### chaos ###
Min: 0.232720 -> 0.271953: 1.17x slower
Avg: 0.234730 -> 0.281721: 1.20x slower
Significant (t=-7.98)
Stddev: 0.00274 -> 0.01289: 4.7072x larger

### django_v3 ###
Min: 0.379254 -> 0.468081: 1.23x slower
Avg: 0.382501 -> 0.470737: 1.23x slower
Significant (t=-50.25)
Stddev: 0.00355 -> 0.00167: 2.1255x smaller

### etree_generate ###
Min: 0.203659 -> 0.230860: 1.13x slower
Avg: 0.205048 -> 0.232591: 1.13x slower
Significant (t=-31.57)
Stddev: 0.00111 -> 0.00161: 1.4500x larger

### etree_iterparse ###
Min: 0.322837 -> 0.345134: 1.07x slower
Avg: 0.324253 -> 0.346395: 1.07x slower
Significant (t=-25.15)
Stddev: 0.00090 -> 0.00175: 1.9341x larger

### etree_process ###
Min: 0.164422 -> 0.177243: 1.08x slower
Avg: 0.165626 -> 0.180134: 1.09x slower
Significant (t=-8.58)
Stddev: 0.00155 -> 0.00345: 2.2218x larger

### fannkuch ###
Min: 0.964744 -> 1.028282: 1.07x slower
Avg: 0.965184 -> 1.038232: 1.08x slower
Significant (t=-22.84)
Stddev: 0.00072 -> 0.00712: 9.8520x larger

### fastpickle ###
Min: 0.367285 -> 0.390253: 1.06x slower
Avg: 0.367853 -> 0.390787: 1.06x slower
Significant (t=-58.26)
Stddev: 0.00064 -> 0.00061: 1.0421x smaller

### fastunpickle ###
Min: 0.471698 -> 0.514821: 1.09x slower
Avg: 0.488927 -> 0.518320: 1.06x slower
Significant (t=-4.79)
Stddev: 0.01327 -> 0.00350: 3.7948x smaller

### float ###
Min: 0.267812 -> 0.251983: 1.06x faster
Avg: 0.270061 -> 0.255820: 1.06x faster
Significant (t=5.68)
Stddev: 0.00177 -> 0.00532: 3.0084x larger

### formatted_logging ###
Min: 0.282892 -> 0.394385: 1.39x slower
Avg: 0.283433 -> 0.406371: 1.43x slower
Significant (t=-11.59)
Stddev: 0.00061 -> 0.02371: 38.7317x larger

### go ###
Min: 0.440843 -> 0.499083: 1.13x slower
Avg: 0.447340 -> 0.500251: 1.12x slower
Significant (t=-19.22)
Stddev: 0.00596 -> 0.00153: 3.9062x smaller

### json_dump_v2 ###
Min: 2.120988 -> 2.308846: 1.09x slower
Avg: 2.127100 -> 2.320961: 1.09x slower
Significant (t=-37.49)
Stddev: 0.00599 -> 0.00989: 1.6507x larger

### nbody ###
Min: 0.246009 -> 0.214377: 1.15x faster
Avg: 0.246976 -> 0.215187: 1.15x faster
Significant (t=41.03)
Stddev: 0.00080 -> 0.00154: 1.9257x larger

### normal_startup ###
Min: 0.520905 -> 9.441582: 18.13x slower
Avg: 0.525316 -> 9.459367: 18.01x slower
Significant (t=-1533.78)
Stddev: 0.00456 -> 0.01220: 2.6771x larger

### pathlib ###
Min: 0.210946 -> 0.225119: 1.07x slower
Avg: 0.216951 -> 0.228365: 1.05x slower
Significant (t=-9.46)
Stddev: 0.00343 -> 0.00496: 1.4447x larger

### raytrace ###
Min: 1.046834 -> 1.142848: 1.09x slower
Avg: 1.048966 -> 1.144411: 1.09x slower
Significant (t=-66.46)
Stddev: 0.00273 -> 0.00169: 1.6134x smaller

### regex_compile ###
Min: 0.260204 -> 0.322876: 1.24x slower
Avg: 0.268089 -> 0.323789: 1.21x slower
Significant (t=-20.16)
Stddev: 0.00611 -> 0.00094: 6.4670x smaller

### richards ###
Min: 0.130934 -> 0.138996: 1.06x slower
Avg: 0.131172 -> 0.139707: 1.07x slower
Significant (t=-25.89)
Stddev: 0.00023 -> 0.00070: 3.0609x larger

### silent_logging ###
Min: 0.060981 -> 0.071124: 1.17x slower
Avg: 0.061093 -> 0.071923: 1.18x slower
Significant (t=-20.71)
Stddev: 0.00009 -> 0.00117: 12.8666x larger

### simple_logging ###
Min: 0.246971 -> 0.342824: 1.39x slower
Avg: 0.247887 -> 0.354878: 1.43x slower
Significant (t=-10.49)
Stddev: 0.00068 -> 0.02280: 33.4399x larger

### spectral_norm ###
Min: 0.288022 -> 0.277858: 1.04x faster
Avg: 0.288412 -> 0.279959: 1.03x faster
Significant (t=6.50)
Stddev: 0.00041 -> 0.00288: 7.0654x larger

### startup_nosite ###
Min: 0.337781 -> 6.046993: 17.90x slower
Avg: 0.343908 -> 6.058127: 17.62x slower
Significant (t=-1946.84)
Stddev: 0.00549 -> 0.00749: 1.3640x larger

### tornado_http ###
Min: 0.414877 -> 0.478739: 1.15x slower
Avg: 0.428162 -> 0.541979: 1.27x slower
Significant (t=-2.47)
Stddev: 0.00968 -> 0.14535: 15.0180x larger

### unpack_sequence ###
Min: 0.000037 -> 0.000179: 4.84x slower
Avg: 0.000039 -> 0.000181: 4.60x slower
Significant (t=-3459.74)
Stddev: 0.00000 -> 0.00000: 1.0377x larger

### unpickle_list ###
Min: 0.370868 -> 0.321671: 1.15x faster
Avg: 0.378591 -> 0.322013: 1.18x faster
Significant (t=10.73)
Stddev: 0.01178 -> 0.00051: 22.9916x smaller

The following not significant results are hidden, use -v to show them:
etree_parse, json_load, mako_v2, meteor_contest, nqueens, pickle_dict, pickle_list, pidigits, regex_effbot, regex_v8, telco.
```
