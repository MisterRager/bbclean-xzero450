# Makefile

OUTPUT = sandbox.exe
LIBS = -lstdc++

OBJS = sandbox.o

all: $(OUTPUT)


$(OUTPUT): $(OBJS)
	gcc -o $(OUTPUT) $(OBJS) $(LIBS)

%.o: %.cpp
	gcc -c -o $@ $^
	
clean:
	rm -rf $(OUTPUT) $(OBJS)