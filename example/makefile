TARGET = benchmark

all: $(TARGET).cpp clean
	g++ $(TARGET).cpp -O3 -Ilib -Ldynamic_lib -lultimate_alpr-sdk -o $(TARGET)
clean:
	rm -f $(TARGET)
