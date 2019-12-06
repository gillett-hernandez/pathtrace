ifeq ($(OS), Windows_NT)
	opts=""
else
	opts="-pthread"
endif

build: ray.cpp ray.h vec3.h main.cpp hittable.h hittable_list.h helpers.h camera.h random.h primitive.h texture.h
	time g++ $(opts) -std=c++14 -O3  main.cpp ray.cpp  -o main.exe -I.

debug:
	time g++ $(opts) -g -std=c++14 main.cpp ray.cpp  -o main.exe -I.

run: build
	time ./main.exe
	python3 convert_ppm_in_curdir.py
clean:
	rm *.o
	rm *.gch
