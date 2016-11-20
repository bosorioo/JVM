all:
	gcc -std=c99 -Wall src/*.c -o jvm.exe -lm
	
debug:
	gcc -std=c99 -Wall src/*.c -DDEBUG -o jvm.exe -lm

test_viewer:
	jvm.exe examples\\LongCode.class -c -b > examples\\LongCode.output.txt
	jvm.exe examples\\HelloWorld.class -c -b > examples\\HelloWorld.output.txt
	
test_interpreter:
	jvm.exe examples/HelloWorld.class -e
	
java:
	javac -encoding utf8 examples/LongCode.java
	javap -v examples/LongCode.class > examples\\LongCode.javap.output.txt
	javac -encoding utf8 examples/HelloWorld.java
	javap -v examples/HelloWorld.class > examples\\HelloWorld.javap.output.txt
	del "examples\\*$$*.class"