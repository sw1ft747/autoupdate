perl generate_app_info.pl Pepe pepe.jpg 1 0 0

IF NOT EXIST "project" (
	mkdir project
)

cd project

cmake .. -A Win32

pause
