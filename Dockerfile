FROM conanio/gcc10

COPY CMakeLists.txt ./
COPY conan.cmake conanfile.txt ./
COPY src ./src
COPY test ./test

RUN cmake -S ./ -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
RUN cmake --build build

EXPOSE 6789
ENTRYPOINT build/tomsksoft-server
