class InstanceMethod {
    private int bonus;
    public InstanceMethod() {
        bonus = 10;
    }
    public int add(int x, int y) {
        return x + y + bonus;
    }
}