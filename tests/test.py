from sys import path as PATH
PATH.append("..")

import unittest
import pathutils

from os import path, W_OK

class TestPathUtils(unittest.TestCase):
   
    existing_dir = path.abspath("./")
    existing_file = path.abspath(__file__)

    non_existing_path = existing_dir + "/invalid_path"
        
    # make generic 
    def test_expand_dir(self):
        self.assertEqual(pathutils.expandpath(path.basename("./file")), 
            "/home/spowell/programming/python/backgrounder/tests/file")
        self.assertEqual(pathutils.expandpath("../file"), 
            "/home/spowell/programming/python/backgrounder/file")
        self.assertEqual(pathutils.expandpath("~/file"), 
            "/home/spowell/file")

    def test_validate_path(self):
        
        self.assertTrue(pathutils.validate_path(self.existing_file, W_OK, create=False))
        self.assertFalse(pathutils.validate_path(self.existing_dir, W_OK, create=False))
        self.assertFalse(pathutils.validate_path(self.non_existing_path, W_OK, create=False))

        self.assertFalse(pathutils.validate_path(self.non_existing_path + "/bad", W_OK, create=True))
        self.assertFalse(pathutils.validate_path(self.existing_dir, W_OK, create=True))

        self.assertTrue(pathutils.validate_path(self.existing_dir + "/file", W_OK, create=True))
        self.assertTrue(pathutils.validate_path(self.existing_file, W_OK, create=True))


if __name__ == '__main__':
    unittest.main()
