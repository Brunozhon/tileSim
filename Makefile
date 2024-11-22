main: main.cpp
	g++ main.cpp -o main -F/Library/Frameworks -framework SDL2 -framework SDL2_image -std=c++17 -Wl,-rpath,/Library/Frameworks -fsanitize=address

# em++ --use-port=sdl2 --use-port=sdl2_image:formats=png main.cpp -o index.html --embed-file img