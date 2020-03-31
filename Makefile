ifeq ($(OS), Windows_NT)
	opts=--std=c++14
else
	opts=-pthread --std=c++14
endif

HPP = $(wildcard *.h) $(wildcard **/*.h)

run: main.exe
	python3 pre_render.py
	./main.exe
	python3 convert_ppm.py

lodepng.o:
	g++ $(opts) -O3 -c thirdparty/lodepng/lodepng.cpp -o lodepng.o

main.exe: main.cpp $(HPP) lodepng.o
	g++ $(opts) -O3 main.cpp lodepng.o -o main.exe -I.

debug: main.cpp $(HPP)
	g++ $(opts) -g main.cpp thirdparty/lodepng/lodepng.cpp -o main.exe -I.
	gdb main.exe

check: main.cpp $(HPP)
	g++ $(opts) main.cpp thirdparty/lodepng/lodepng.cpp -o main.exe -I.

strict: main.cpp $(HPP)
	g++ $(opts) -Wall -Wpedantic main.cpp thirdparty/lodepng/lodepng.cpp -o main.exe -I.

test.exe: test.cpp mesh_loader.h mesh.h triangle.h
	g++ $(opts) test.cpp -o test.exe -I.

test: test.exe
	./test.exe



run_w_pillow: main.exe
	python3 pre_render.py
	./main.exe
	python3 -m pip install Pillow
	python3 convert_ppm.py

run_and_send: run_w_pillow
	python3 -m pip install sendgrid
	python3 send_result.py

clean:
	rm *.o || echo
	rm *.gch || echo
	rm main.exe || echo

.PHONY: run run_w_pillow clean run_and_send strict test
