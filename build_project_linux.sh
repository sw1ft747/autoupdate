perl generate_app_info.pl Pepe pepe.jpg 1 0 0

[ ! -d ./build ] && mkdir build

cd build

cmake .. -DCMAKE_BUILD_TYPE=RELEASE
make
