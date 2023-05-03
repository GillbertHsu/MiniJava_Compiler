class Base {
    public static int method() {
        return 1;
    }

	public static void main(String[] args) {
        System.out.println(args[0]);
        int a = 4 + method();
        System.out.println(a);
	}
}
