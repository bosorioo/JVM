all:
	gcc -std=c99 -Wall attributes.c constantpool.c fields.c javaclassfile.c main.c methods.c readfunctions.c -o class_viewer.exe