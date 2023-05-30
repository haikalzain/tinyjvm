class NativeBasic {
    public static native int addInt(int x, int y);
    public static native float addFloat(float x, float y);
    public static void main(String[] args) {
        new NativeBasic().getClass();
    }
}