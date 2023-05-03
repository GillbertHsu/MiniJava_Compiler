class Base {
	public static int method(int x) {
		return x + 1;
	}
	
	public static void main(String[] args){
		System.out.println(args[0]);
		int a = method(4) + (2 * 3 - (9 - 4));
		System.out.println(a);
	}
}
