javac -target 11 Exceptions.java
javac -target 11 FunctionCalls.java
javac -target 11 SelfInitialize.java
javac -target 11 NativeBasic.java
javac -target 11 ArrayTest.java
javac -target 11 --patch-module java.base=. java/lang/Object.java