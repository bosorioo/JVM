all:
	gcc -m32 -std=c99 -Wall src/*.c -o jvm.exe -lm
	
debug:
	gcc -std=c99 -Wall src/*.c -DDEBUG -o jvmdebug.exe -lm

test_viewer:
	jvm.exe examples\LongCode.class -c -b > examples\LongCode.output.txt
	jvm.exe examples\HelloWorld.class -c -b > examples\HelloWorld.output.txt
	jvm.exe examples\NameTest.class -c -b > examples\NameTest.output.txt
	
test_interpreter:
	jvm.exe examples/HelloWorld.class -e
	
.PHONY: java
java: 
	javac -encoding utf8 examples/LongCode.java
	javap -v examples/LongCode.class > examples/LongCode.javap.output.txt
	javac -encoding utf8 examples/HelloWorld.java
	javap -v examples/HelloWorld.class > examples/HelloWorld.javap.output.txt
	javac -encoding utf8 examples/NameTest.java
	javap -v examples/NameTest.class > examples/NameTest.javap.output.txt
	del "examples\\*$$*.class"