package sketch;

import java.util.*;
import javafx.geometry.Point2D;

public class Sketch implements EventListener, RandomAccess {

	private static final int[] array_member = {1, 2, 3};
	@Deprecated
	public float deprecated_member;
	public static long static_long_member = 98765432123456789L;
	private String πφμβƒé;

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
		public int method1(int[] a);
	}
	
	class NestedClass implements Test {
		public int method1(int[] a) {
			return a.length;
		}
	}

	public void mymethod(int i, long n) {
		int[] a = new int[2];
		NestedClass nc = new NestedClass();
		// To force invokeinterface instruction
		a[0] = ((Test)nc).method1(a);
		n += static_long_member;
	}

	public long arith(int a, float b, byte c) throws Error {

		switch (a) {
			case 10: b += 20; break;
			case 30: b -= 20; break;
			default:
				break;
		}
		
		switch (c) {
			case 2: a += b; break;
			case 3: a -= b; break;
			case 4: a *= b; break;
			case 5: a /= b; break;
			default:
				throw new Error();
		}
		
		return (long)a;
	}

	public static void main(String[] args) {
		Object obj = (Object)"ola";
		
		if (obj instanceof String) {
			System.out.println((String)obj);
		}
		
		String[][] stringMatrix = new String[2][2];
		
		int num1, num2;
		try { 
			num1 = 0;
			num2 = 62 / num1;
		} catch (ArithmeticException e) { 
		}
	}

}
