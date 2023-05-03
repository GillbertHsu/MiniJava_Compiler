class Base {    
    public static int method1(int b) {
        System.out.println(b);
        int x = method2(6);
        return b - x;
    }
    
    public static int method2(int a) {
        System.out.println(a);
        return 6;
    }

	public static void main(String[] args){
        System.out.println(args[0]);
        int a = method1(19);
        System.out.println(a);
	}
}
