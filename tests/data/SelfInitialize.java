public class SelfInitialize extends SelfInitializeParent {
    static int x;
    static int x2;
    static SelfInitialize y;
    static {
        x2 = 2;
        y = new SelfInitialize();
        x = 2;
    }

    public SelfInitialize() {
        x += 1;
        x2 += 1;
    }

    public static void main(String[] args) {
        System.out.println(SelfInitialize.x);
    }

    public static int test() {
        return SelfInitialize.x;
    }

     public static int test2() {
        return SelfInitialize.x2;
    }

    public static int test3() {
        return SelfInitialize.parentX;
    }

}