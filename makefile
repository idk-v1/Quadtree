make:	main.cpp
	g++ *.cpp `sdl-config --cflags --libs`

fast:	main.cpp
	g++ *.cpp `sdl-config --cflags --libs` -O3
