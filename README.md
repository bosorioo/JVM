# Java Virtual Machine

A project made for the course "Basic Software" of University of Brasilia.  
This software can read and print the contents or execute compiled Java programs.

# Compiling

To compile, use the provided Makefile by running the following command:

```make```

This will generate the binary "jvm" in the root folder.  
It is also possible to compile a debug version, by using the following command:

```make debug```

The debug version prints detailed information of the code being executed, files being loaded and other general info.  
The generated binary for the debug version is named "jvmdebug".

# Documentation

Most of the source code includes Doxygen documentation.  
The auto generated doc files are located in the ```doxygen``` folder.  
The documentation can be viewed in this link: https://bosorioo.github.io/JVM/

# Execution

To print the contents of a .class file, use the following command:

```./jvm my_compiled_java.class -c```

Examples of the generated output of .class content dumping by this software can be found in the [examples folder](https://github.com/bosorioo/JVM/tree/master/examples).  
Naming convention of that folder is as follow:
- &lt;ProgramName>.java: Java source code sample
- &lt;ProgramName>.class: Compiled Java binary
- &lt;ProgramName>.javap.output.txt: Dump of compiled Java program using ```javap```
- &lt;ProgramName>.output.txt: Dump of compiled Java program using this software

To execute a .class file, use the following command:

```./jvm my_compiled_java.class -e```

The same applies with the debug version "jvmdebug".
