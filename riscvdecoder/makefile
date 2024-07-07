all: riscvdecoder 

riscvdecoder:
	g++   -g -O3 -I/home/vivsg/projects/systemc/include riscvdecoder.cpp elf_parser.cpp -L/home/vivsg/projects/systemc/lib-linux64 -Wl,-rpath=/home/vivsg/projects/systemc/lib-linux64 -lsystemc -lm -lelf -lbfd -o riscvdecoder.elf

run:
	./riscvdecoder.elf

clean:
	rm -rf *.o *.elf
