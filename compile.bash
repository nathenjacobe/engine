cd build
rm -rf *
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ..
make -j4
cd ..
