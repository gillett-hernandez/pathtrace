ifeq ($(OS), Windows_NT)
	opts=--std=c++14
else
	opts=-pthread --std=c++14
endif

main.exe: ray.h vec3.h main.cpp hittable.h hittable_list.h helpers.h camera.h random.h primitive.h texture.h scene.h material.h
	g++ $(opts) -O3 main.cpp -o main.exe -I.

debug:
	g++ $(opts) -g  main.cpp  -o main.exe -I.

check:
	g++ $(opts) main.cpp -o main.exe -I.

strict:
	g++ $(opts) -Wall -Wpedantic main.cpp -o main.exe -I.

run: main.exe
	./main.exe

run_w_pillow: main.exe
	./main.exe
	python3 -m pip install Pillow
	(python3 convert_ppm.py &)


run_and_send: run_w_pillow
	python3 -m pip install sendgrid
	python3 send_result.py

clean:
	rm *.o
	rm *.gch

.PHONY: run run_w_pillow clean run_and_send strict
