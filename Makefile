
build: ray.cpp ray.h vec3.h main.cpp hittable.h hittable_list.h helpers.h camera.h random.h sphere.h
	g++ -O3 main.cpp ray.cpp  -o main.exe -I.

run: build
	./main.exe > out.ppm
clean:
	rm *.o
