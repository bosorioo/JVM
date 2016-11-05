package sketch;

import java.util.*;

public class Sketch implements EventListener, RandomAccess {

	private static final int[] values = {1, 2, 3, 4, 5};
	
	public long long_value = 65536 * 65536 * 655356 * 21;
	public static long long_value_static = 218372183L * 1238L;
	
	public double[][] matrix = {
		{123.456, -654.321, 987654321.123456789},
		{Double.MAX_VALUE, Double.MIN_VALUE, Double.MIN_NORMAL},
		{Double.NaN, Double.POSITIVE_INFINITY, Double.NEGATIVE_INFINITY}
	};
	
	interface Test {
		public void method1(int[] a);
	}

    public void print(int i) {
        System.out.println(i);
    }
	
	public long add(int a, float b) {
		long result = (long)a + (long)b;
		return result;
	}
    
    public static void main(String[] args) {
        System.out.println("Hello world!");
    }
    
}
