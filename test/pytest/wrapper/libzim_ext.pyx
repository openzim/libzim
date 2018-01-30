cimport zim_wrapper as zim

cdef class File:
    cdef zim.File c_file

    def __cinit__(self, bytes filename):
        self.c_file = zim.File(filename)

    def verify(self):
        return self.c_file.verify()
