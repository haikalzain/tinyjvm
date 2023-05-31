public class ArrayTest {
    public static int testSingleDim() {
        int[] arr = new int[3];
        arr[0] = 1;
        arr[1] = 2;
        arr[2] = 3;
        return arr[0] + arr[1] + arr[2];
    }

    public static int testMultiDim() {
        int[][] arr = new int[10][20];
        arr[1][2] = 10;
        int[] arr2 = arr[1];

        return arr2[2] + arr[0][0];
    }


    public static void main(String[] args) {
        System.out.println(testSingleDim());
        System.out.println(testMultiDim());
    }
}