perl generate_app_info.pl Pepe pepe.jpg 1 0 0

IF NOT EXIST "build" (
	mkdir build
)

cd build

cmake .. -A Win32
cmake --build . --config Release

pause