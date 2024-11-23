g++ ./src/app.cpp -o ./build/app.out `pkg-config --cflags --libs opencv4`

if [[ "$1" == "--run" ]]; then
    # Run source
    ./build/app.out
fi
