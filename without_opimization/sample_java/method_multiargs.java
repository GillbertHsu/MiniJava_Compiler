class Base{
    public static int method(int a, int b) {
        System.out.print(a);
        System.out.println(b);
        int x = a + b;
        return x;
    }
	public static void main(String[] args){
        System.out.println(args[0]);
        int a = method(7, 3);
        System.out.println(a);
	}
}
