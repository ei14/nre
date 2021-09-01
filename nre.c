// Copyright (c) 2021 Thomas Kaldahl

#include "nre.h"
#include "config.h"

const int COUNT01 = COUNT0 + COUNT1;
const int COUNT012 = COUNT01 + COUNT2;

typedef scalar (*f0)(void);
typedef scalar (*f1)(scalar);
typedef scalar (*f2)(scalar, scalar);
f0 func0[] = FUNC0;
f1 func1[] = FUNC1;
f2 func2[] = FUNC2;

char *genexpr(long n, int mod) {
	char *res;
	if(n == 0) {
		res = malloc(2);
		res[0] = 'A';
		res[1] = '\0';
		return res;
	}

	res = malloc(3 + (int)floor(log(n)/log(mod)));
	int rp = 0;
	while(n > 0) {
		res[rp++] = 'A' + n % mod;
		n /= COUNT012;
	}
	res[rp] = '\0';
	return res;
}

scalar eval(char *expr) {
	int exprlen = strlen(expr);
	scalar *stack = malloc((exprlen + 1) * sizeof(scalar));
	int sp = -1;

	for(int i = 0; i < exprlen; i++) {
		int sel = expr[i] - 'A';

		if(sel >= COUNT0 && sp == -1) {
			free(stack);
			return DEFAULT;
		}
		if(sel >= COUNT01 && sp == 0) {
			free(stack);
			return DEFAULT;
		}

		if(sel < COUNT0) {
			stack[++sp] = func0[sel]();
		} else if(sel < COUNT01) {
			stack[sp] = func1[sel - COUNT0](stack[sp]);
		} else {
			sp--;
			stack[sp] = func2[sel - COUNT01](stack[sp], stack[sp+1]);
		}
	}
	scalar res = stack[0];
	free(stack);
	if(sp != 0) return DEFAULT;
	return res;
}

typedef struct comparison {
	char *expr;
	scalar value;
	double error;
	struct comparison *left;
	struct comparison *right;
	int protected;
} comparison;

comparison *findBest(comparison *root) {
	if(root->left) return(findBest(root->left));
	return root;
}
int insertElem(comparison *root, comparison *elem) {
	if(elem->error > root->error) {
		if(root->right) {
			return insertElem(root->right, elem);
		} else {
			root->right = elem;
			return 1;
		}
	}
	if(elem->error < root->error) {
		if(root->left) {
			return insertElem(root->left, elem);
		} else {
			root->left = elem;
			return 1;
		}
	}
	free(elem->expr);
	free(elem);
	return 0;
}
void linearize(comparison *root, comparison **linearized, int *start, int end) {
	if(root && *start < end) {
		linearize(root->left, linearized, start, end);
		if(*start < end) {
			root->protected = 1;
			linearized[*start] = root;
			(*start)++;
		}
		linearize(root->right, linearized, start, end);
	}
}
void freeTree(comparison *root) {
	if(root) {
		freeTree(root->left);
		freeTree(root->right);
		if(root->protected) {
			root->protected = 0;
		} else {
			free(root->expr);
			free(root);
		}
	}
}
void freeTreeForce(comparison *root) {
	if(root) {
		freeTreeForce(root->left);
		freeTreeForce(root->right);
		free(root->expr);
		free(root);
	}
}

typedef struct matchParams {
	comparison **roots;
	int tid;
	comparison ***res;
} matchParams;

void *findMatches(void *params) {
	comparison **roots = ((matchParams*)params)->roots;
	int tid = ((matchParams*)params)->tid;
	comparison ***res = ((matchParams*)params)->res;

	roots[tid] = malloc(sizeof(comparison));
	roots[tid]->expr = genexpr(tid, COUNT012);
	roots[tid]->value = eval(roots[tid]->expr);
	roots[tid]->error = magnitude(TARGET - DEFAULT);
	roots[tid]->left = roots[tid]->right = NULL;
	roots[tid]->protected = 0;

	long rescount = 1;
	clock_t now, then;
	now = then = clock();

	for(long i = 1; i < LIMIT/THREADS; i++) {
		if(i % PROGRESS == 0) {
			now = clock();
			double timeSince = now - then;
			comparison *best = findBest(roots[tid]);
			char *formatted = format(best->value);
			fflush(stdout);
			printf(
				"\033[%d;H\033[K %.0lds\t%s\t%s\t(%.12f)\r",
				tid+1,
				(long) timeSince * (LIMIT/THREADS - i) / PROGRESS / 1000000 / THREADS,
				best->expr,
				formatted,
				magnitude(best->value - TARGET)
			);
			free(formatted);
			then = now;
		}

		comparison *result = malloc(sizeof(comparison));
		long n = i*THREADS + tid;
		result->expr = genexpr(n, COUNT012);
		result->value = eval(result->expr);
		if(result->value == DEFAULT) {
			free(result->expr);
			free(result);
		} else {
			result->error = magnitude(result->value - TARGET);
			result->left = result->right = NULL;
			roots[tid]->protected = 0;

			if(insertElem(roots[tid], result)) rescount++;
		}

		if(rescount >= CLEANUP) {
			comparison **kept = malloc(KEEP * sizeof(comparison*));
			int *start = malloc(sizeof(int));
			*start = 0;
			linearize(roots[tid], kept, start, KEEP);
			free(start);
			freeTree(roots[tid]);

			roots[tid] = kept[0];
			kept[KEEP-1]->left = kept[KEEP-1]->right = NULL;
			for(int j = 0; j < KEEP-1; j++) {
				kept[j]->left = NULL;
				kept[j]->right = kept[j+1];
			}

			rescount = KEEP;
		}
	}

	res[tid] = malloc(KEEP * sizeof(comparison*));
	int *start = malloc(sizeof(int));
	*start = 0;
	linearize(roots[tid], res[tid], start, KEEP);
	free(start);

	return NULL;
}

int main(void) {
	printf("\033[;H\033[J");

	comparison **roots = malloc(THREADS * sizeof(comparison*));
	comparison ***res = malloc(THREADS * sizeof(comparison**));

	pthread_t *threads = malloc(THREADS * sizeof(pthread_t));
	matchParams **params = malloc(THREADS * sizeof(matchParams*));
	for(int tid = 0; tid < THREADS; tid++) {
		params[tid] = malloc(sizeof(matchParams));
		params[tid]->tid = tid;
		params[tid]->roots = roots;
		params[tid]->res = res;
		pthread_create(&threads[tid], NULL, findMatches, (void*)(params[tid]));
	}
	for(int tid = 0; tid < THREADS; tid++) {
		pthread_join(threads[tid], NULL);
	}
	for(int tid = 0; tid < THREADS; tid++) {
		free(params[tid]);
	}
	free(params);
	free(threads);

	printf("\033[;H\033[JExpression\tValue\t\t(Error)\n");
	for(int pass = 0; pass < KEEP; pass++) {
		int besttid = -1;
		int besti = -1;
		double record = -1;
		for(int tid = 0; tid < THREADS; tid++) {
			for(int i = 0; i < KEEP; i++) {
				if(
					res[tid][i]
					&& res[tid][i]->error != -1
					&& (record == -1 || res[tid][i]->error < record)
				) {
					besttid = tid;
					besti = i;
					record = res[tid][i]->error;
				}
			}
		}
		char *formatted = format(res[besttid][besti]->value);
		printf(
			"%s\t%s\t(%.12f)\n",
			res[besttid][besti]->expr,
			formatted,
			res[besttid][besti]->error
		);
		free(formatted);
		res[besttid][besti]->error = -1;
	}

	for(int tid = 0; tid < THREADS; tid++) {
		freeTreeForce(roots[tid]);
		free(res[tid]);
	}
	free(roots);
	free(res);
}
