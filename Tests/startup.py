import gc
gc.disable()
import pyjion
pyjion.enable()
from test.libregrtest import main
main()