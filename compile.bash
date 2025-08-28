cd build
rm -rf *
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ..
cmake --build .
cd ..
./Renderer
