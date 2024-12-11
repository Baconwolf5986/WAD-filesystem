libWad.a: Wad.o
	ar cr libWad.a Wad.o

Wad.o: Wad.cpp Wad.h
	g++ -std=c++17 -c Wad.cpp -o Wad.o