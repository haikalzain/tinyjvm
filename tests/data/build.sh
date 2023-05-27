javac -target 11 Basic.java
javac -target 11 FunctionCalls.java
javac -target 11 --patch-module java.base=. java/lang/Object.java