class Base{
    public static int method() {
        return 5;
    }
    
	public static void main(String[] args){
        System.out.println(args[0]);
        int a = +method();
		System.out.println(a);
	}
}
