# Build application
g++ ./src/app.cpp -o ./build/app.out `pkg-config --cflags --libs opencv4`

# Run source
./build/app.out
