call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\VC\Auxiliary\Build\vcvars64.bat"

set CC=cl.exe
set CXX=cl.exe

meson.exe setup build . --force-fallback-for liblzma -Ddefault_library=static -Dwith_xapian=disabled -Dzstd:bin_programs=false -Dzstd:bin_tests=false -Dzstd:bin_contrib=false -Dliblzma:default_library=static -Dliblzma:enable_xz=false

cd build

ninja.exe
