
build: ray.cpp ray.h vec3.h main.cpp hittable.h hittable_list.h helpers.h camera.h random.h sphere.h
	time g++ -std=c++14 -O3  main.cpp ray.cpp  -o main.exe -I.

debug:
	time g++ -g -std=c++14 main.cpp ray.cpp  -o main.exe -I.

run: build
	time ./main.exe > out.ppm
	python3 convert_ppm_in_curdir.py
clean:
	rm *.o
