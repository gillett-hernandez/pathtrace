ifeq ($(OS), Windows_NT)
	opts=--std=c++14
else
	opts=-pthread --std=c++14
endif

build: ray.h vec3.h main.cpp hittable.h hittable_list.h helpers.h camera.h random.h primitive.h texture.h
	time g++ $(opts) -O3 main.cpp -o main.exe -I.

debug:
	time g++ $(opts) -g  main.cpp  -o main.exe -I.

run: build
	time ./main.exe
	python3 -m pip install Pillow
	(python3 convert_ppm_in_curdir.py &)
clean:
	rm *.o
	rm *.gch
