show: 
	cat Makefile
compile_proto: 
	mkdir -p generated
	protoc --cpp_out=./generated obstacle_code_path_report.proto
compile: compile_proto
	g++ main.cc obstacle_code_path_report.cc generated/obstacle_code_path_report.pb.cc -I generated `pkg-config --cflags --libs protobuf`  -o main
run:
	./main
clean:
	rm generated main -rf