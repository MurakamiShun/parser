func test(){
	1 + 2 + 3 + 4;
	1 - 2 + 3 - 4;
	1 + (2 - 3 + 1);
	1 + (2 - (3 - 4));

	1 * 2 + 3 * 4;
	1 * 2 * 3 * 4;
	1 / 2 + 3 / 4;
	1 - 2 * 3 / 4;
	(1 + 2) * (3 + 4);
	
	1 + test;
	test + 1;
	1 + test * 2;
	1 - test(2+3, test(4+5));
	test() - 4325;
	1 * test() + 2 * 3;
}