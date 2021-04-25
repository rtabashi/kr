#define NULL	0

typedef long Align;		/* longの境界に整合させる */

union header {			/* ブロックのヘッダ */
	struct {
		union header *ptr;	/* 空リストの上なら次のブロック */
		unsigned size;		/* このブロックの大きさ */
	} s;
	Align x;			/* ブロックの整合を強制 */
};

typedef union header Header;

static Header base;			/* 開始時の空きリスト */
static Header *freep = NULL;	/* 空きリストの先頭 */

/* krfree: ブロックapを空きリストに入れる */
void krfree(void *ap)
{
	Header *bp, *p;

	bp = (Header *)ap - 1;		/* ブロック・ヘッダを指す */
	for (p = freep; !(bp > p && bp < p->s.ptr); p = p->s.ptr)
	   if (p >= p->s.ptr && (bp > p || bp < p->s.ptr))
			break;	/* 領域の始めあるいは終りの解放ブロック */

	if (bp + bp->s.size == p->s.ptr) {	/* 上のnbrへ結合 */
		bp->s.size += p->s.ptr->s.size;
		bp->s.ptr = p->s.ptr->s.ptr;
	} else
		bp->s.ptr = p->s.ptr;
	if (p + p->s.size == bp) {			/* 下のnbrへ結合 */
		p->s.size += bp->s.size;
		p->s.ptr = bp->s.ptr;
	} else
		p->s.ptr = bp;
	freep = p;
}

#define NALLOC	1024	/* 要求する最小の単位数 */

/* morecore: システムにもっとメモリを要求する */
static Header *morecore(unsigned nu)
{
	char *cp, *sbrk(int);
	Header *up;

	if (nu < NALLOC)
		nu = NALLOC;
	cp = sbrk(nu * sizeof(Header));
	if (cp == (char *) -1)	/* スペースが全然ない */
		return NULL;
	up = (Header *) cp;
	up->s.size = nu;
	krfree((void *)(up+1));
	return freep;
}

/* krmalloc: 汎用の記憶割当てプログラム */
void *krmalloc(unsigned nbytes)
{
	Header *p, *prevp;
	Header *morecore(unsigned);
	unsigned nunits;

	nunits = (nbytes+sizeof(Header)-1)/sizeof(Header) + 1;
	if ((prevp = freep) == NULL) {	/* 空きリストはまだない */
		base.s.ptr = freep = prevp = &base;
		base.s.size = 0;
	}
	for (p = prevp->s.ptr; ; prevp = p, p = p->s.ptr) {
		if (p->s.size >= nunits) {	/* 十分大きい */
			if (p->s.size == nunits)	/* 正確に */
				prevp->s.ptr = p->s.ptr;
			else {					/* 後尾の部分を割り当て */
				p->s.size -= nunits;
				p += p->s.size;
				p->s.size = nunits;
			}
			freep = prevp;
			return (void *)(p+1);
		}
		if (p == freep)	/* 空きリストをリング状につなぐ */
			if ((p = morecore(nunits)) == NULL)
				return NULL;	/* 残りなし */
	}
}
