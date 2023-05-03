class Base {
    public static int method1(int b) {
        System.out.println(b);
        return 9;
    }

    public static int method2(int a) {
        System.out.println(a);
        return 4;
    }
    
	public static void main(String[] args){
        System.out.println(args[0]);
        int a = method1(9) - method2(4);
        System.out.println(a);
	}
}
