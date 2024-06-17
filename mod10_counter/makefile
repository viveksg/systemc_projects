all: mod10counter 

mod10counter:
	g++ -g -O3 -I/home/vivsg/projects/systemc/include -L/home/vivsg/projects/systemc/lib-linux64 -Wl,-rpath=/home/vivsg/projects/systemc/lib-linux64 -lsystemc -lm mod10counter.cpp -o m10counter.elf

run:
	./m10counter.elf

clean:
	rm -rf *.o *.elf
