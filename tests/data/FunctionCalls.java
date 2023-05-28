class FunctionCalls {
    public static int add(int x, int y) {
        return x + y;
    }

    public static int add2() {
        int x = add(2, 4);
        InstanceMethod y = new InstanceMethod();
        x = y.add(x, 5);
        return x;
    }
}