
build: ray.cpp ray.h vec3.h main.cpp
	g++ -O2 main.cpp vec3.h ray.cpp ray.h  -o main.exe -I.

run: build
	./main.exe > out.ppm
clean:
	rm *.o
