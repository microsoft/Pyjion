from pyjion.dis import print_il
import unittest
import io
import contextlib


class test_dis(unittest.TestCase):

    @unittest.expectedFailure  # not implemented yet
    def test_fat(self):
        test_method = bytearray(b'\x00\x00\x00\x00\x00\x00\x00\x00\xa68\xd6\x11\xa5\x7f\x00\x00\x00\x00\x00\x00\x00'
                                b'\x00\x00\x00\xa68\xd6\x11\xa5\x7f\x00\x00\x0e8\xd6\x11\xa5\x7f\x00\x00\n\x00\x00'
                                b'\x00J\x17XT\x90\xdd\xdb\x99\xff\x7f\x00\x00\x08\x00\x00\x00\x00\x00\x00\x00\x00\x00'
                                b'\x00\x00 h\x01\x00\x00\xd3X\x11\n\xdf(\x10\x00\x00\x00\x06 '
                                b'\x04\x00\x00\x00\xd3T!P\x19\xd2\x11\xa5\x7f\x00\x00\xd3% '
                                b'\x00\x00\x00\x00\xd3X%J\x17XT\x06 \x06\x00\x00\x00\xd3T\x13\n\x03 '
                                b'p\x01\x00\x00\xd3XM\x03 p\x01\x00\x00\xd3X\x11\n\xdf(\x10\x00\x00\x00\x06 '
                                b'\x08\x00\x00\x00\xd3T\x03 '
                                b'h\x01\x00\x00\xd3XM%\x0c\x16\xd3@\x1a\x00\x00\x00!0\x1f\xee\x11\xa5\x7f\x00\x00'
                                b'\xd3(:\x00\x00\x00\x03(8\x00\x00\x008\n\x04\x00\x00\x08% '
                                b'\x00\x00\x00\x00\xd3X%J\x17XT\x06 \n\x00\x00\x00\xd3T\x03 '
                                b'p\x01\x00\x00\xd3XM%\x0c\x16\xd3@\x1f\x00\x00\x00!p\xbe\xe1\x11\xa5\x7f\x00\x00'
                                b'\xd3(:\x00\x00\x00\x03(8\x00\x00\x00(\x10\x00\x00\x008\xc3\x03\x00\x00\x08% '
                                b'\x00\x00\x00\x00\xd3X%J\x17XT\x06 \x0c\x00\x00\x00\xd3T\x18('
                                b'\t\x00\x00\x00%\x0c\x16\xd3@\x0b\x00\x00\x00\x03('
                                b'8\x00\x00\x008\x93\x03\x00\x00\x08\x06 '
                                b'\x0e\x00\x00\x00\xd3T%!\x80\x9a\x91\t\x01\x00\x00\x00\xd3;=\x00\x00\x00%!`\x9a\x91'
                                b'\t\x01\x00\x00\x00\xd3;#\x00\x00\x00%(\x05\x00\x02\x00%\x15@\x11\x00\x00\x00&\x03('
                                b'8\x00\x00\x00(\x10\x00\x00\x008L\x03\x00\x00:\n\x00\x00\x00('
                                b'\x10\x00\x00\x008T\x00\x00\x00(\x10\x00\x00\x00\x06 '
                                b'\x10\x00\x00\x00\xd3T!p\x19\xd2\x11\xa5\x7f\x00\x00\xd3% '
                                b'\x00\x00\x00\x00\xd3X%J\x17XT\x06 \x12\x00\x00\x00\xd3T\x13\n\x03 '
                                b'x\x01\x00\x00\xd3XM\x03 '
                                b'x\x01\x00\x00\x00\x00\x00\x00\x00\xe0\xb2\x83\x13\xa5\x7f\x00\x00\xe0\xb2\x83\x13'
                                b'\xa5\x7f\x00\x00\x01\x00\x00\x00\x16\x00\x00\x00\x00\x00\x00\x00('
                                b'\xd2\x11\xa5\x7f\x00\x00\xd3% \x00\x00\x00\x00\xd3X%J\x17XT\x06 '
                                b'\x18\x00\x00\x00\xd3T\x13\n\x03 x\x01\x00\x00\xd3XM\x03 '
                                b'x\x01\x00\x00\xd3X\x11\n\xdf(\x10\x00\x00\x00\x06 \x1a\x00\x00\x00\xd3T\x03 '
                                b'x\x01\x00\x00\xd3XM%\x0c\x16\xd3@\x1a\x00\x00\x00!0\x8c\xdd\x11\xa5\x7f\x00\x00'
                                b'\xd3(:\x00\x00\x00\x03(8\x00\x00\x008s\x02\x00\x00\x08% '
                                b'\x00\x00\x00\x00\xd3X%J\x17XT\x06 '
                                b'\x1c\x00\x00\x00\xd3T!\x90\x19\xd2\x11\xa5\x7f\x00\x00\xd3% '
                                b'\x00\x00\x00\x00\xd3X%J\x17XT\x06 \x1e\x00\x00\x00\xd3T\x18('
                                b'\t\x00\x00\x00%\x0c\x16\xd3@\x0b\x00\x00\x00\x03('
                                b'8\x00\x00\x008$\x02\x00\x00\x08\x06  '
                                b'\x00\x00\x00\xd3T%!\x80\x9a\x01\x00\x00\x00\x00\x00\xd3;=\x00\x00\x00%!`\x9a\x91\t'
                                b'\x01\x00\x00\x00\xd3;#\x00\x00\x00%(\x05\x00\x02\x00%\x15@\x11\x00\x00\x00&\x03('
                                b'8\x00\x00\x00(\x10\x00\x00\x008\xdd\x01\x00\x00:\n\x00\x00\x00('
                                b'\x10\x00\x00\x008\x8d\x00\x00\x00(\x10\x00\x00\x00\x06 '
                                b'"\x00\x00\x00\xd3T\x03!\xb0\xc6\xd6\x11\xa5\x7f\x00\x00\xd3('
                                b'\x00\x00\x03\x00%\x0c\x16\xd3@\x0b\x00\x00\x00\x03('
                                b'8\x00\x00\x008\x9d\x01\x00\x00\x08\x06 '
                                b'$\x00\x00\x00\xd3T!\xf0\xb2\x83\x13\xa5\x7f\x00\x00\xd3% '
                                b'\x00\x00\x00\x00\xd3X%J\x17XT\x06 &\x00\x00\x00\xd3T('
                                b'\x01\x00\x01\x00%\x0c\x16\xd3@\x0b\x00\x00\x00\x03('
                                b'8\x00\x00\x008\\\x01\x00\x00\x08\x06 (\x00\x00\x00\xd3T(\x10\x00\x00\x00\x06 '
                                b'*\x00\x00\x00\xd3T8{\x00\x00\x00\x06 ,'
                                b'\x00\x00\x00\xd3T\x03!\xb0\xc6\xd6\x11\xa5\x7f\x00\x00\xd3('
                                b'\x00\x00\x03\x00%\x0c\x16\xd3@\x0b\x00\x00\x00\x03('
                                b'8\x00\x00\x008\x15\x01\x00\x00\x08\x06 '
                                b'.\x00\x00\x00\xd3T!\xb0\xb5\x83\x13\xa5\x7f\x00\x00\xd3% '
                                b'\x00\x00\x00\x00\xd3X%J\x17XT\x06 0\x00\x00\x00\xd3T('
                                b'\x01\x00\x01\x00%\x0c\x16\xd3@\x0b\x00\x00\x00\x03('
                                b'8\x00\x00\x008\xd4\x00\x00\x00\x08\x06 2\x00\x00\x00\xd3T(\x10\x00\x00\x00\x06 '
                                b'4\x00\x00\x00\xd3T!0\x19\xd2\x11\xa5\x7f\x00\x00\xd3% '
                                b'\x00\x00\x00\x00\xd3X%J\x17XT\x06 6\x00\x00\x00\xd3T\x13\n\x03 '
                                b'\x80\x01\x00\x00\xd3XM\x03 \x80\x01\x00\x00\xd3X\x11\n\xdf(\x10\x00\x00\x00\x06 '
                                b'8\x00\x00\x00\xd3T!P\x19\xd2\x11\xa5\x7f\x00\x00\xd3% '
                                b'\x00\x00\x00\x00\xd3X%J\x17XT\x06 :\x00\x00\x00\xd3T\x13\n\x03 '
                                b'\x88\x01\x00\x00\xd3XM\x03 \x88\x01\x00\x00\xd3X\x11\n\xdf(\x10\x00\x00\x00\x06 '
                                b'<\x00\x00\x00\xd3T!\xe0\xce\x92\t\x01\x00\x00\x00\xd3% '
                                b'\x00\x00\x00\x00\xd3X%J\x17XT\x06 '
                                b'>\x00\x00\x00\xd3T\x0b\xdd\x1c\x00\x00\x00\t\x16>\t\x00\x00\x00&&&\x19\tY\r+\xf08'
                                b'\x00\x00\x00\x00\x16\xd38\x01\x00\x00\x00\x07\x03(B\x00\x00\x00*')
        f = io.StringIO()
        with contextlib.redirect_stdout(f):
            print_il(test_method)
        self.assertIn("ldarg.1", f.getvalue())

    def test_thin(self):
        test_method = bytearray(b'\x03 h\x00\x00\x00\xd3X\n\x03(A\x00\x00\x00\x16\r\x06 '
                                b'\x00\x00\x00\x00\xd3T\x03!\xb0\xc6V)\x91\x7f\x00\x00\xd3('
                                b'\x00\x00\x03\x00%\x0c\x16\xd3@\x0b\x00\x00\x00\x03('
                                b'8\x00\x00\x008\x91\x00\x00\x00\x08\x06 '
                                b'\x02\x00\x00\x00\xd3T!\xf0\xc3\x13*\x91\x7f\x00\x00\xd3% '
                                b'\x00\x00\x00\x00\xd3X%J\x17XT\x06 \x04\x00\x00\x00\xd3T('
                                b'\x01\x00\x01\x00%\x0c\x16\xd3@\x0b\x00\x00\x00\x03('
                                b'8\x00\x00\x008P\x00\x00\x00\x08\x06 \x06\x00\x00\x00\xd3T(\x10\x00\x00\x00\x06 '
                                b'\x08\x00\x00\x00\xd3T!\xe0\x1e\xda\x02\x01\x00\x00\x00\xd3% '
                                b'\x00\x00\x00\x00\xd3X%J\x17XT\x06 '
                                b'\n\x00\x00\x00\xd3T\x0b\xdd\x1c\x00\x00\x00\t\x16>\t\x00\x00\x00&&&\x19\tY\r+\xf08'
                                b'\x00\x00\x00\x00\x16\xd38\x01\x00\x00\x00\x07\x03(B\x00\x00\x00*')
        f = io.StringIO()
        with contextlib.redirect_stdout(f):
            print_il(test_method)
        self.assertIn("ldarg.1", f.getvalue())


if __name__ == "__main__":
    unittest.main()