import pyjion
import gc
pyjion.enable()
from test.libregrtest import main
main()
gc.collect()
pyjion.disable()
print("Disabling JIT")
print(pyjion.stats())