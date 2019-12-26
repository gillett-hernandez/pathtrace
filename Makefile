ifeq ($(OS), Windows_NT)
	opts=--std=c++14
else
	opts=-pthread --std=c++14
endif

HPP = $(wildcard *.h) $(wildcard **/*.h)

main.exe: main.cpp $(HPP)
	g++ $(opts) -O3 main.cpp -o main.exe -I.

debug: main.cpp $(HPP)
	g++ $(opts) -g  main.cpp  -o main.exe -I.

check: main.cpp $(HPP)
	g++ $(opts) main.cpp -o main.exe -I.

check_strict: main.cpp $(HPP)
	g++ $(opts) -Wall -Wpedantic main.cpp -o main.exe -I.

safe_run: main.exe
	./main.exe
	python3 -m pip install Pillow
	(python3 convert_ppm_in_curdir.py &)

run: main.exe
	./main.exe
	python3 convert_ppm_in_curdir.py

run_and_send: run
	python3 -m pip install sendgrid
	python3 send_result.py

clean:
	rm *.o || echo
	rm *.gch || echo
	rm main.exe || echo

.PHONY: run safe_run debug clean run_and_send check check_strict
