package sketch;

import java.util.*;
import javafx.geometry.Point2D;

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
		public int method1(int[] a);
	}
	
	interface AnotherInterface {
		public void interfaceMethod();
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
		n += long_static_value;
	}

	public long add_10(int a, float b) throws Error {
		a += 5;
		long result = (long)a + (long)b;
		result += 5;
		return result;
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
