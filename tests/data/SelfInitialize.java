public class SelfInitialize {
    static int x;
    static SelfInitialize y;
    static {

        y = new SelfInitialize();
        x = 2;
    }

    public SelfInitialize() {
        x += 1;
    }

    public static void main(String[] args) {
        System.out.println(SelfInitialize.x);
    }

    public static int test() {
        return SelfInitialize.x;
    }

}