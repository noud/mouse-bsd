# $NetBSD: extest.awk,v 1.1 1999/05/11 19:14:50 drochner Exp $

BEGIN {
	first = 1;

	printf("#include <stdio.h>\n")
	printf("#include <sys/extent.h>\n\n")
	printf("main() {\n")
	printf("struct extent *ex; int error; long result;\n")
}

END {
	printf("exit (0);\n")
	printf("}\n")
}

$1 == "extent" {
	if (first == 0) {
		printf("extent_destroy(ex);\n")
	}

	align = "EX_NOALIGN";
	boundary = "EX_NOBOUNDARY";

	printf("printf(\"output for %s\\n\");\n", $2)

	if ($5 == "") {
		flags = "0";
	} else {
		flags = $5;
	}
	printf("ex = extent_create(\"%s\", %s, %s, 0, 0, 0, %s);\n",
	       $2, $3, $4, flags)

	first = 0;
}

$1 == "alloc_region" {
	printf("error = extent_alloc_region(ex, %s, %s, 0);\n",
	       $2, $3)
	printf("if (error)\n\tprintf(\"error: %%s\\n\", strerror(error));\n")
}

$1 == "alloc_subregion" {
	printf("error = extent_alloc_subregion1(ex, %s, %s, %s,\n",
	       $2, $3, $4)
	printf("\t%s, 0, %s, 0, &result);\n", align, boundary)
	printf("if (error)\n\tprintf(\"error: %%s\\n\", strerror(error));\n")
	printf("else\n\tprintf(\"result: 0x%%x\\n\", result);\n")
}

$1 == "print" {
	printf("extent_print(ex);\n")
}
