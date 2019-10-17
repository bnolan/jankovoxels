
compile:
	emcc core.cpp -std=c++11 -s FETCH=1 -s WASM=1 -lsdl2 -L lib -I lib -I include -O3 -o index.js
