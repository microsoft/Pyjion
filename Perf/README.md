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
C:\Source\benchmarks>py -3 perf.py -r --csv ..\Pyjion5\Perf\2016-04-06.csv --benchmarks=all,-chameleon_v2,-hexiom2 C:\Source\Pyjion5\Python\PCbuild\nojit\python.exe C:\Source\Pyjion5\Python\PCbuild\amd64\python.exe
INFO:root:Automatically selected timer: perf_counter
INFO:root:Skipping benchmark rietveld; not compatible with Python 3.5
INFO:root:Skipping benchmark slowunpickle; not compatible with Python 3.5
INFO:root:Skipping benchmark html5lib_warmup; not compatible with Python 3.5
INFO:root:Skipping benchmark bzr_startup; not compatible with Python 3.5
INFO:root:Skipping benchmark slowpickle; not compatible with Python 3.5
INFO:root:Skipping benchmark slowspitfire; not compatible with Python 3.5
INFO:root:Skipping benchmark html5lib; not compatible with Python 3.5
INFO:root:Skipping benchmark pybench; not compatible with Python 3.5
INFO:root:Skipping benchmark spambayes; not compatible with Python 3.5
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
Min: 12.342356 -> 334.583629: 27.11x slower
Avg: 12.550288 -> 339.405125: 27.04x slower
Significant (t=-140.37)
Stddev: 0.19301 -> 5.20306: 26.9573x larger

### call_method ###
Min: 0.489932 -> 1.018983: 2.08x slower
Avg: 0.524157 -> 1.089300: 2.08x slower
Significant (t=-141.23)
Stddev: 0.03192 -> 0.06152: 1.9273x larger

### call_method_slots ###
Min: 0.500812 -> 1.014403: 2.03x slower
Avg: 0.515659 -> 1.091850: 2.12x slower
Significant (t=-174.00)
Stddev: 0.00833 -> 0.05675: 6.8136x larger

### call_method_unknown ###
Min: 0.499555 -> 1.686318: 3.38x slower
Avg: 0.514263 -> 1.801308: 3.50x slower
Significant (t=-176.69)
Stddev: 0.00450 -> 0.12609: 28.0335x larger

### call_simple ###
Min: 0.412433 -> 0.477337: 1.16x slower
Avg: 0.420309 -> 0.526400: 1.25x slower
Significant (t=-101.69)
Stddev: 0.00301 -> 0.01782: 5.9156x larger

### chaos ###
Min: 0.526826 -> 0.638008: 1.21x slower
Avg: 0.537415 -> 0.701176: 1.30x slower
Significant (t=-171.90)
Stddev: 0.00409 -> 0.00861: 2.1058x larger

### django_v3 ###
Min: 0.958868 -> 7.305971: 7.62x slower
Avg: 0.979457 -> 7.367174: 7.52x slower
Significant (t=-1369.64)
Stddev: 0.00793 -> 0.04596: 5.7933x larger

### etree_generate ###
Min: 0.531633 -> 3.110812: 5.85x slower
Avg: 0.547371 -> 3.160219: 5.77x slower
Significant (t=-1417.84)
Stddev: 0.00594 -> 0.01744: 2.9355x larger

### etree_iterparse ###
Min: 0.725201 -> 12.410611: 17.11x slower
Avg: 0.748303 -> 12.507122: 16.71x slower
Significant (t=-3390.54)
Stddev: 0.00874 -> 0.03356: 3.8394x larger

### etree_parse ###
Min: 0.469335 -> 0.509480: 1.09x slower
Avg: 0.480375 -> 0.524586: 1.09x slower
Significant (t=-55.20)
Stddev: 0.00550 -> 0.00582: 1.0589x larger

### etree_process ###
Min: 0.446792 -> 2.110003: 4.72x slower
Avg: 0.461770 -> 2.157288: 4.67x slower
Significant (t=-906.59)
Stddev: 0.01052 -> 0.01546: 1.4699x larger

### fastpickle ###
Min: 0.789316 -> 0.812318: 1.03x slower
Avg: 0.806700 -> 0.830384: 1.03x slower
Significant (t=-25.98)
Stddev: 0.00700 -> 0.00584: 1.1981x smaller

### float ###
Min: 0.495348 -> 0.586380: 1.18x slower
Avg: 0.517097 -> 0.601411: 1.16x slower
Significant (t=-70.16)
Stddev: 0.01015 -> 0.00644: 1.5754x smaller

### formatted_logging ###
Min: 0.727788 -> 0.745749: 1.02x slower
Avg: 0.741796 -> 0.997155: 1.34x slower
Significant (t=-76.83)
Stddev: 0.00493 -> 0.03287: 6.6701x larger

### go ###
Min: 0.967717 -> 1.297788: 1.34x slower
Avg: 0.988757 -> 1.328556: 1.34x slower
Significant (t=-286.16)
Stddev: 0.00734 -> 0.00933: 1.2706x larger

### json_dump_v2 ###
Min: 4.728246 -> 5.931067: 1.25x slower
Avg: 4.788513 -> 5.989510: 1.25x slower
Significant (t=-350.81)
Stddev: 0.02435 -> 0.02406: 1.0122x smaller

### json_load ###
Min: 0.741040 -> 0.772750: 1.04x slower
Avg: 0.758066 -> 0.829820: 1.09x slower
Significant (t=-67.69)
Stddev: 0.00552 -> 0.00905: 1.6379x larger

### mako_v2 ###
Min: 0.068379 -> 0.075004: 1.10x slower
Avg: 0.071573 -> 0.078239: 1.09x slower
Significant (t=-95.28)
Stddev: 0.00164 -> 0.00149: 1.0980x smaller

### meteor_contest ###
Min: 0.323812 -> 0.373890: 1.15x slower
Avg: 0.332440 -> 0.392416: 1.18x slower
Significant (t=-93.76)
Stddev: 0.00522 -> 0.00370: 1.4127x smaller

### normal_startup ###
Min: 0.924916 -> 1.038923: 1.12x slower
Avg: 0.937876 -> 1.050537: 1.12x slower
Significant (t=-86.34)
Stddev: 0.00951 -> 0.00894: 1.0632x smaller

### nqueens ###
Min: 0.456231 -> 40.414209: 88.58x slower
Avg: 0.469850 -> 40.692625: 86.61x slower
Significant (t=-1064.20)
Stddev: 0.00483 -> 0.37793: 78.2330x larger

### pathlib ###
Min: 0.461617 -> 0.477238: 1.03x slower
Avg: 0.469790 -> 1.027322: 2.19x slower
Significant (t=-231.79)
Stddev: 0.00366 -> 0.05366: 14.6753x larger

### pidigits ###
Min: 0.409860 -> 0.445579: 1.09x slower
Avg: 0.422306 -> 0.538591: 1.28x slower
Significant (t=-44.29)
Stddev: 0.00795 -> 0.02502: 3.1491x larger

### raytrace ###
Min: 2.384660 -> 3.441256: 1.44x slower
Avg: 2.423691 -> 3.494174: 1.44x slower
Significant (t=-537.58)
Stddev: 0.01222 -> 0.01572: 1.2863x larger

### regex_compile ###
Min: 0.605274 -> 0.738788: 1.22x slower
Avg: 0.619787 -> 0.793985: 1.28x slower
Significant (t=-41.38)
Stddev: 0.00499 -> 0.04180: 8.3806x larger

### regex_v8 ###
Min: 0.076342 -> 0.076527: 1.00x slower
Avg: 0.078724 -> 0.081784: 1.04x slower
Significant (t=-9.71)
Stddev: 0.00228 -> 0.00218: 1.0457x smaller

### richards ###
Min: 0.283325 -> 0.371891: 1.31x slower
Avg: 0.291840 -> 0.384265: 1.32x slower
Significant (t=-218.60)
Stddev: 0.00299 -> 0.00299: 1.0028x larger

### silent_logging ###
Min: 0.121146 -> 0.190932: 1.58x slower
Avg: 0.125954 -> 0.204959: 1.63x slower
Significant (t=-221.26)
Stddev: 0.00143 -> 0.00327: 2.2797x larger

### simple_logging ###
Min: 0.641986 -> 0.656115: 1.02x slower
Avg: 0.656170 -> 0.875481: 1.33x slower
Significant (t=-74.50)
Stddev: 0.00452 -> 0.02909: 6.4341x larger

### spectral_norm ###
Min: 0.584597 -> 0.707676: 1.21x slower
Avg: 0.598195 -> 0.730366: 1.22x slower
Significant (t=-51.50)
Stddev: 0.00546 -> 0.02507: 4.5949x larger

### startup_nosite ###
Min: 0.610495 -> 0.705637: 1.16x slower
Avg: 0.618553 -> 0.717202: 1.16x slower
Significant (t=-109.85)
Stddev: 0.00866 -> 0.00929: 1.0727x larger

### tornado_http ###
Min: 0.740326 -> 0.766377: 1.04x slower
Avg: 0.753335 -> 2.597251: 3.45x slower
Significant (t=-23.61)
Stddev: 0.00679 -> 1.10450: 162.6085x larger

### unpickle_list ###
Min: 0.629430 -> 0.611556: 1.03x faster
Avg: 0.662157 -> 0.645913: 1.03x faster
Significant (t=4.98)
Stddev: 0.01909 -> 0.02647: 1.3864x larger

The following not significant results are hidden, use -v to show them:
fannkuch, fastunpickle, nbody, pickle_dict, pickle_list, regex_effbot, telco, unpack_sequence.
```
