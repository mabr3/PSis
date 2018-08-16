CXX= gcc $(CCFLAGS)

LIBS= -pthread

CCFLAGS= -g -Wall

all: app_teste clipboard

app_teste: app_teste.c
	$(CXX) app_teste.c library.c clipboard.h -o app_teste

clipboard: clipboard.c clipboard.h
	$(CXX) $(LIBS) clipboard.c  clipboard.h -o clipboard

clean:
	rm -f *.o clipboard app_teste
