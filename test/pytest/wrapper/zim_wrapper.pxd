from libcpp.string cimport string

cdef extern from "../../../include/zim/file.h" namespace "zim":
    cdef cppclass File:
        File() except +
        File(string filename) except +

        bint verify()
