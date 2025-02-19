FROM gcc:4.9
ARG PROGRAM_ID
COPY . /usr/src/myapp
WORKDIR /usr/src/myapp
RUN gcc -o app tmp/${PROGRAM_ID}.c -std=c11
CMD ["./app"]