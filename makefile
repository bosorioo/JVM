all:
	gcc -std=c99 -Wall *.c -o class_viewer.exe -lm

teste:
	class_viewer.exe Sketch.class utf8bom > output.txt
	notepad.exe output.txt
	
java:
	javac -encoding utf8 Sketch.java
	del "*$$*.class"
	javap -v Sketch.class > javap.output.txt
	notepad.exe javap.output.txt