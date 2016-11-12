package sketch;

import java.util.*;

public class Sketch implements EventListener, RandomAccess {

	private static final int[] values = {1, 2, 3, 4, 5};

	@Deprecated
	public float float_member;
	
	public long long_member = 5910974510923776L;
	public static long long_static_value = 98765432123456789L;

	private String πφμβƒé = "πφμβƒ";
	protected final boolean bool_member = true;

	private int [][][] matrix3d = {
		{{1, 2},
		{3, 4}},
		{{5, 6},
		{7, 8}}
	};

	public double[][] matrix2d = {
		{123.456, -654.321, 987654321.123456789},
		{Double.MAX_VALUE, Double.MIN_VALUE, Double.MIN_NORMAL},
		{Double.NaN, Double.POSITIVE_INFINITY, Double.NEGATIVE_INFINITY}
	};

	interface Test {
		public void method1(int[] a);
	}
	
	interface AnotherInterface {
		
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
