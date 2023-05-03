class Base {
    public static int method2(int a) {
        System.out.println(a);
        return 7;
    }
    
    public static int method1(int b) {
        System.out.println(b);
        int x = method2(7);
        return b - x;
    }

	public static void main(String[] args){
        System.out.println(args[0]);
        int a = method1(10);
        System.out.println(a);
	}
}
