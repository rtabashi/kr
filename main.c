#define	SIZE	100

void krfree(void *ap);
void *krmalloc(unsigned nbytes);

int main(void)
{
	int *p;

	p = krmalloc(SIZE);
	krfree(p);
}
