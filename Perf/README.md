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
C:\Source\benchmarks>py -3 perf.py -r --csv ..\Pyjion5\Perf\2016-05-13.csv --benchmarks=all,-chameleon_v2,-hexiom2 C:\Source\Pyjion5\Python\PCbuild\nojit\python.exe C:\Source\Pyjion5\Python\PCbuild\amd64\python.exe
INFO:root:Automatically selected timer: perf_counter
INFO:root:Skipping benchmark html5lib_warmup; not compatible with Python 3.5
INFO:root:Skipping benchmark slowunpickle; not compatible with Python 3.5
INFO:root:Skipping benchmark slowpickle; not compatible with Python 3.5
INFO:root:Skipping benchmark slowspitfire; not compatible with Python 3.5
INFO:root:Skipping benchmark spambayes; not compatible with Python 3.5
INFO:root:Skipping benchmark bzr_startup; not compatible with Python 3.5
INFO:root:Skipping benchmark rietveld; not compatible with Python 3.5
INFO:root:Skipping benchmark html5lib; not compatible with Python 3.5
INFO:root:Skipping benchmark pybench; not compatible with Python 3.5
INFO:root:Skipping benchmark hg_startup; not compatible with Python 3.5
[ 1/41] 2to3...
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\amd64\python.exe lib3/2to3/2to3 -f all lib/2to3`
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\amd64\python.exe lib3/2to3/2to3 -f all lib/2to3` 5 times
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\nojit\python.exe lib3/2to3/2to3 -f all lib/2to3`
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\nojit\python.exe lib3/2to3/2to3 -f all lib/2to3` 5 times
[ 2/41] call_method...
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\amd64\python.exe performance/bm_call_method.py -n 300 --timer perf_counter`
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\nojit\python.exe performance/bm_call_method.py -n 300 --timer perf_counter`
[ 3/41] call_method_slots...
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\amd64\python.exe performance/bm_call_method_slots.py -n 300 --timer perf_counter`
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\nojit\python.exe performance/bm_call_method_slots.py -n 300 --timer perf_counter`
[ 4/41] call_method_unknown...
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\amd64\python.exe performance/bm_call_method_unknown.py -n 300 --timer perf_counter`
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\nojit\python.exe performance/bm_call_method_unknown.py -n 300 --timer perf_counter`
[ 5/41] call_simple...
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\amd64\python.exe performance/bm_call_simple.py -n 300 --timer perf_counter`
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\nojit\python.exe performance/bm_call_simple.py -n 300 --timer perf_counter`
[ 6/41] chaos...
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\amd64\python.exe performance/bm_chaos.py -n 100 --timer perf_counter`
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\nojit\python.exe performance/bm_chaos.py -n 100 --timer perf_counter`
[ 7/41] django_v3...
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\amd64\python.exe performance/bm_django_v3.py -n 100 --timer perf_counter`
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\nojit\python.exe performance/bm_django_v3.py -n 100 --timer perf_counter`
[ 8/41] etree_generate...
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\amd64\python.exe performance/bm_elementtree.py -n 100 --timer perf_counter generate`
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\nojit\python.exe performance/bm_elementtree.py -n 100 --timer perf_counter generate`
[ 9/41] etree_iterparse...
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\amd64\python.exe performance/bm_elementtree.py -n 100 --timer perf_counter iterparse`
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\nojit\python.exe performance/bm_elementtree.py -n 100 --timer perf_counter iterparse`
[10/41] etree_parse...
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\amd64\python.exe performance/bm_elementtree.py -n 100 --timer perf_counter parse`
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\nojit\python.exe performance/bm_elementtree.py -n 100 --timer perf_counter parse`
[11/41] etree_process...
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\amd64\python.exe performance/bm_elementtree.py -n 100 --timer perf_counter process`
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\nojit\python.exe performance/bm_elementtree.py -n 100 --timer perf_counter process`
[12/41] fannkuch...
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\amd64\python.exe performance/bm_fannkuch.py -n 100 --timer perf_counter`
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\nojit\python.exe performance/bm_fannkuch.py -n 100 --timer perf_counter`
[13/41] fastpickle...
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\amd64\python.exe performance/bm_pickle.py -n 100 --timer perf_counter --use_cpickle pickle`
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\nojit\python.exe performance/bm_pickle.py -n 100 --timer perf_counter --use_cpickle pickle`
[14/41] fastunpickle...
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\amd64\python.exe performance/bm_pickle.py -n 100 --timer perf_counter --use_cpickle unpickle`
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\nojit\python.exe performance/bm_pickle.py -n 100 --timer perf_counter --use_cpickle unpickle`
[15/41] float...
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\amd64\python.exe performance/bm_float.py -n 100 --timer perf_counter`
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\nojit\python.exe performance/bm_float.py -n 100 --timer perf_counter`
[16/41] formatted_logging...
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\amd64\python.exe performance/bm_logging.py -n 100 --timer perf_counter formatted_output`
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\nojit\python.exe performance/bm_logging.py -n 100 --timer perf_counter formatted_output`
[17/41] go...
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\amd64\python.exe performance/bm_go.py -n 100 --timer perf_counter`
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\nojit\python.exe performance/bm_go.py -n 100 --timer perf_counter`
[18/41] json_dump_v2...
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\amd64\python.exe performance/bm_json_v2.py -n 100 --timer perf_counter`
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\nojit\python.exe performance/bm_json_v2.py -n 100 --timer perf_counter`
[19/41] json_load...
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\amd64\python.exe performance/bm_json.py -n 100 --timer perf_counter json_load`
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\nojit\python.exe performance/bm_json.py -n 100 --timer perf_counter json_load`
[20/41] mako_v2...
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\amd64\python.exe performance/bm_mako_v2.py -n 1000 --timer perf_counter`
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\nojit\python.exe performance/bm_mako_v2.py -n 1000 --timer perf_counter`
[21/41] meteor_contest...
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\amd64\python.exe performance/bm_meteor_contest.py -n 100 --timer perf_counter`
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\nojit\python.exe performance/bm_meteor_contest.py -n 100 --timer perf_counter`
[22/41] nbody...
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\amd64\python.exe performance/bm_nbody.py -n 100 --timer perf_counter`
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\nojit\python.exe performance/bm_nbody.py -n 100 --timer perf_counter`
[23/41] normal_startup...
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\amd64\python.exe -c ` 2000 times
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\nojit\python.exe -c ` 2000 times
[24/41] nqueens...
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\amd64\python.exe performance/bm_nqueens.py -n 100 --timer perf_counter`
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\nojit\python.exe performance/bm_nqueens.py -n 100 --timer perf_counter`
[25/41] pathlib...
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\amd64\python.exe performance/bm_pathlib.py -n 1000 --timer perf_counter`
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\nojit\python.exe performance/bm_pathlib.py -n 1000 --timer perf_counter`
[26/41] pickle_dict...
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\amd64\python.exe performance/bm_pickle.py -n 100 --timer perf_counter --use_cpickle pickle_dict`
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\nojit\python.exe performance/bm_pickle.py -n 100 --timer perf_counter --use_cpickle pickle_dict`
[27/41] pickle_list...
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\amd64\python.exe performance/bm_pickle.py -n 100 --timer perf_counter --use_cpickle pickle_list`
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\nojit\python.exe performance/bm_pickle.py -n 100 --timer perf_counter --use_cpickle pickle_list`
[28/41] pidigits...
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\amd64\python.exe performance/bm_pidigits.py -n 100 --timer perf_counter`
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\nojit\python.exe performance/bm_pidigits.py -n 100 --timer perf_counter`
[29/41] raytrace...
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\amd64\python.exe performance/bm_raytrace.py -n 100 --timer perf_counter`
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\nojit\python.exe performance/bm_raytrace.py -n 100 --timer perf_counter`
[30/41] regex_compile...
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\amd64\python.exe performance/bm_regex_compile.py -n 100 --timer perf_counter`
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\nojit\python.exe performance/bm_regex_compile.py -n 100 --timer perf_counter`
[31/41] regex_effbot...
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\amd64\python.exe performance/bm_regex_effbot.py -n 100 --timer perf_counter`
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\nojit\python.exe performance/bm_regex_effbot.py -n 100 --timer perf_counter`
[32/41] regex_v8...
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\amd64\python.exe performance/bm_regex_v8.py -n 100 --timer perf_counter`
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\nojit\python.exe performance/bm_regex_v8.py -n 100 --timer perf_counter`
[33/41] richards...
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\amd64\python.exe performance/bm_richards.py -n 100 --timer perf_counter`
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\nojit\python.exe performance/bm_richards.py -n 100 --timer perf_counter`
[34/41] silent_logging...
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\amd64\python.exe performance/bm_logging.py -n 100 --timer perf_counter no_output`
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\nojit\python.exe performance/bm_logging.py -n 100 --timer perf_counter no_output`
[35/41] simple_logging...
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\amd64\python.exe performance/bm_logging.py -n 100 --timer perf_counter simple_output`
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\nojit\python.exe performance/bm_logging.py -n 100 --timer perf_counter simple_output`
[36/41] spectral_norm...
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\amd64\python.exe performance/bm_spectral_norm.py -n 100 --timer perf_counter`
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\nojit\python.exe performance/bm_spectral_norm.py -n 100 --timer perf_counter`
[37/41] startup_nosite...
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\amd64\python.exe -S -c ` 4000 times
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\nojit\python.exe -S -c ` 4000 times
[38/41] telco...
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\amd64\python.exe performance/bm_telco.py -n 100 --timer perf_counter`
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\nojit\python.exe performance/bm_telco.py -n 100 --timer perf_counter`
[39/41] tornado_http...
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\amd64\python.exe performance/bm_tornado_http.py -n 200 --timer perf_counter`
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\nojit\python.exe performance/bm_tornado_http.py -n 200 --timer perf_counter`
[40/41] unpack_sequence...
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\amd64\python.exe performance/bm_unpack_sequence.py -n 100000 --timer perf_counter`
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\nojit\python.exe performance/bm_unpack_sequence.py -n 100000 --timer perf_counter`
[41/41] unpickle_list...
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\amd64\python.exe performance/bm_pickle.py -n 100 --timer perf_counter --use_cpickle unpickle_list`
INFO:root:Running `C:\Source\Pyjion5\Python\PCbuild\nojit\python.exe performance/bm_pickle.py -n 100 --timer perf_counter --use_cpickle unpickle_list`

Report on Windows dinov1  10.0.10586 AMD64 Intel64 Family 6 Model 44 Stepping 2, GenuineIntel
Total CPU cores: 8

### 2to3 ###
Min: 12.115672 -> 53.077635: 4.38x slower
Avg: 12.183010 -> 53.180146: 4.37x slower
Significant (t=-748.60)
Stddev: 0.07164 -> 0.09931: 1.3862x larger

### call_method ###
Min: 0.478239 -> 0.440329: 1.09x faster
Avg: 0.485248 -> 0.445719: 1.09x faster
Significant (t=46.08)
Stddev: 0.01410 -> 0.00467: 3.0169x smaller

### call_method_slots ###
Min: 0.477080 -> 0.437468: 1.09x faster
Avg: 0.485912 -> 0.443677: 1.10x faster
Significant (t=63.83)
Stddev: 0.00811 -> 0.00809: 1.0026x smaller

### call_method_unknown ###
Min: 0.496415 -> 0.447550: 1.11x faster
Avg: 0.504634 -> 0.460771: 1.10x faster
Significant (t=22.54)
Stddev: 0.00914 -> 0.03244: 3.5512x larger

### call_simple ###
Min: 0.411499 -> 0.376116: 1.09x faster
Avg: 0.419098 -> 0.381976: 1.10x faster
Significant (t=44.53)
Stddev: 0.01151 -> 0.00871: 1.3213x smaller

### chaos ###
Min: 0.529725 -> 0.508128: 1.04x faster
Avg: 0.538782 -> 0.515974: 1.04x faster
Significant (t=13.77)
Stddev: 0.01030 -> 0.01297: 1.2603x larger

### django_v3 ###
Min: 0.962259 -> 1.973536: 2.05x slower
Avg: 0.974137 -> 2.000700: 2.05x slower
Significant (t=-312.54)
Stddev: 0.02231 -> 0.02410: 1.0803x larger

### etree_generate ###
Min: 0.555797 -> 0.878412: 1.58x slower
Avg: 0.580327 -> 0.894168: 1.54x slower
Significant (t=-83.09)
Stddev: 0.03371 -> 0.01705: 1.9775x smaller

### etree_iterparse ###
Min: 0.744044 -> 2.469276: 3.32x slower
Avg: 0.758416 -> 2.527216: 3.33x slower
Significant (t=-266.50)
Stddev: 0.00904 -> 0.06575: 7.2708x larger

### etree_process ###
Min: 0.455153 -> 0.655453: 1.44x slower
Avg: 0.465813 -> 0.681561: 1.46x slower
Significant (t=-50.27)
Stddev: 0.01111 -> 0.04145: 3.7317x larger

### float ###
Min: 0.517170 -> 0.478662: 1.08x faster
Avg: 0.528597 -> 0.490600: 1.08x faster
Significant (t=40.38)
Stddev: 0.00535 -> 0.00774: 1.4455x larger

### formatted_logging ###
Min: 0.736918 -> 0.756467: 1.03x slower
Avg: 0.749573 -> 0.771023: 1.03x slower
Significant (t=-14.29)
Stddev: 0.01237 -> 0.00850: 1.4545x smaller

### go ###
Min: 0.967658 -> 0.871361: 1.11x faster
Avg: 0.978361 -> 0.894362: 1.09x faster
Significant (t=112.95)
Stddev: 0.00443 -> 0.00597: 1.3492x larger

### json_load ###
Min: 0.764063 -> 0.785226: 1.03x slower
Avg: 0.775503 -> 0.792612: 1.02x slower
Significant (t=-25.77)
Stddev: 0.00571 -> 0.00340: 1.6801x smaller

### mako_v2 ###
Min: 0.072189 -> 0.073789: 1.02x slower
Avg: 0.074399 -> 0.076339: 1.03x slower
Significant (t=-36.85)
Stddev: 0.00111 -> 0.00124: 1.1175x larger

### meteor_contest ###
Min: 0.331122 -> 0.306164: 1.08x faster
Avg: 0.342458 -> 0.313908: 1.09x faster
Significant (t=16.03)
Stddev: 0.01397 -> 0.01105: 1.2646x smaller

### nbody ###
Min: 0.456943 -> 0.461399: 1.01x slower
Avg: 0.463576 -> 0.474350: 1.02x slower
Significant (t=-3.93)
Stddev: 0.00746 -> 0.02638: 3.5380x larger

### nqueens ###
Min: 0.456503 -> 5.844960: 12.80x slower
Avg: 0.466118 -> 6.422858: 13.78x slower
Significant (t=-128.84)
Stddev: 0.00829 -> 0.46225: 55.7447x larger

### pathlib ###
Min: 0.466209 -> 0.505793: 1.08x slower
Avg: 0.474145 -> 0.590590: 1.25x slower
Significant (t=-137.63)
Stddev: 0.00606 -> 0.01792: 2.9569x larger

### pickle_list ###
Min: 0.450111 -> 0.439026: 1.03x faster
Avg: 0.454997 -> 0.443361: 1.03x faster
Significant (t=37.54)
Stddev: 0.00237 -> 0.00199: 1.1920x smaller

### pidigits ###
Min: 0.417503 -> 0.433454: 1.04x slower
Avg: 0.423859 -> 0.438408: 1.03x slower
Significant (t=-15.25)
Stddev: 0.00874 -> 0.00383: 2.2800x smaller

### raytrace ###
Min: 2.400198 -> 2.188586: 1.10x faster
Avg: 2.426518 -> 2.228595: 1.09x faster
Significant (t=41.24)
Stddev: 0.01697 -> 0.04490: 2.6457x larger

### regex_compile ###
Min: 0.610075 -> 0.590271: 1.03x faster
Avg: 0.617661 -> 0.602807: 1.02x faster
Significant (t=5.59)
Stddev: 0.00480 -> 0.02612: 5.4446x larger

### richards ###
Min: 0.279421 -> 0.223735: 1.25x faster
Avg: 0.283808 -> 0.227727: 1.25x faster
Significant (t=163.70)
Stddev: 0.00222 -> 0.00261: 1.1797x larger

### silent_logging ###
Min: 0.122719 -> 0.111339: 1.10x faster
Avg: 0.126007 -> 0.114450: 1.10x faster
Significant (t=47.85)
Stddev: 0.00139 -> 0.00197: 1.4203x larger

### spectral_norm ###
Min: 0.571884 -> 0.343651: 1.66x faster
Avg: 0.577222 -> 0.350683: 1.65x faster
Significant (t=224.37)
Stddev: 0.00262 -> 0.00975: 3.7191x larger

### startup_nosite ###
Min: 0.614642 -> 0.666916: 1.09x slower
Avg: 0.639071 -> 0.678824: 1.06x slower
Significant (t=-16.63)
Stddev: 0.03336 -> 0.00549: 6.0789x smaller

### tornado_http ###
Min: 0.746220 -> 0.789006: 1.06x slower
Avg: 0.769729 -> 1.965457: 2.55x slower
Significant (t=-15.37)
Stddev: 0.00869 -> 1.10034: 126.5668x larger

### unpickle_list ###
Min: 0.618535 -> 0.610952: 1.01x faster
Avg: 0.659739 -> 0.621155: 1.06x faster
Significant (t=10.38)
Stddev: 0.03700 -> 0.00337: 10.9645x smaller

The following not significant results are hidden, use -v to show them:
etree_parse, fannkuch, fastpickle, fastunpickle, json_dump_v2, normal_startup, pickle_dict, regex_effbot, regex_v8, simple_logging, telco, unpack_sequence.
```
